// useful stuff
// adb -d logcat | grep SDL/APP
// python3 -m http.server --directory=buildweb/src/platforms/sdl2/
//

#include "main.h"
#include "touch.h"
#include "audio.h"
#include "types.h"

#include <romloader.h>
#include <util.h>
#include <mgb.h>

#include <gb.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>


#include <SDL.h>

#ifdef __SWITCH__
    #include <switch.h>
#endif

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

static emu_t emu = {0};

static void* pixels_buffers[2] = {0};
static void* backbuffer = NULL;
static void* frontbuffer = NULL;

static SDL_mutex* mutex = NULL;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Texture* prev_texture = NULL;
static SDL_Rect rect = {0};
static SDL_PixelFormat* pixel_format = NULL;
static SDL_GameController* game_controller = NULL;
static SDL_DisplayMode display_mode = {0};
static SDL_RendererInfo renderer_info = {0};
static SDL_version version_compiled = {0};
static SDL_version version_linked = {0};
static uint32_t pixel_format_enum = 0;

static void core_vblank_callback(void* user);
static void sdl2_ctrl_key_event(const SDL_KeyboardEvent* e, bool down);
static void sdl2_key_event(const SDL_KeyboardEvent* e);
static void sdl2_touch_event(const SDL_TouchFingerEvent* e);
static void sdl2_window_event(const SDL_WindowEvent* e);

static void on_resize(int w, int h);

static void run(double delta);
static void render(void);
static void events(void);
static void cleanup(void);
static void update_game_texture(void);

#ifdef ANDROID
#include <jni.h>
#define JAVA_PREFIX                                     org_libsdl_totalgb
#define CONCAT1(prefix, class, function)                CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function)                Java_ ## prefix ## _ ## class ## _ ## function
#define JAVA_NAME(function)     CONCAT1(JAVA_PREFIX, totalgbActivity, function)

enum AndroidRequestCode {
    AndroidRequestCode_ROM,
    AndroidRequestCode_SAVE,
    AndroidRequestCode_STATE,
};

static void android_handle_exception(JNIEnv* env)
{
    // we have to save the exception first
    // then clear as other methods cannot be called with exception
    jthrowable exc = (*env)->ExceptionOccurred(env);
    (*env)->ExceptionClear(env);

    jboolean isCopy = false;
    jmethodID toString = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/lang/Object"), "toString", "()Ljava/lang/String;");
    jstring s = (jstring)(*env)->CallObjectMethod(env, exc, toString);
    const char* utf = (*env)->GetStringUTFChars(env, s, &isCopy);

    log_info("[android_file_picker] exception: %s\n", utf);

    (*env)->ReleaseStringUTFChars(env, s, utf);
}

static void android_caller(const char* method_name, enum AndroidRequestCode request_code)
{
    JNIEnv* env = SDL_AndroidGetJNIEnv();
    if (!env)
    {
        log_info("[android_file_picker] failed to get env\n");
        return;
    }

    jobject obj = SDL_AndroidGetActivity();
    if (!obj)
    {
        log_info("[android_file_picker] failed to get obj\n");
        return;
    }

    jclass cls = (*env)->GetObjectClass(env, obj);
    if (!cls)
    {
        log_info("[android_file_picker] failed to get class\n");
        return;
    }

    jmethodID method = (*env)->GetMethodID(env, cls, method_name, "(I)V");
    if (!method)
    {
        log_info("[android_file_picker] failed to get method\n");
        return;
    }

    (*env)->CallVoidMethod(env, obj, method, request_code);

    if ((*env)->ExceptionCheck(env))
    {
        android_handle_exception(env);
    }

    (*env)->DeleteLocalRef(env, obj);
}

void android_file_picker(void)
{
    android_caller("openFile", AndroidRequestCode_ROM);
}

void android_settings_menu(void)
{
    android_caller("openSettings", 0);
}

void android_folder_picker(void)
{
    // android_caller("openFolder");
}

JNIEXPORT void JNICALL JAVA_NAME(OpenFileCallback)(JNIEnv* env, jobject thiz, int fd, bool own_fd, const jstring string)
{
    const jsize str_len = (*env)->GetStringUTFLength(env, string);
    log_info("[OpenFileCallback] size: %d\n", str_len);

    char* file_path = (char*)SDL_malloc(str_len + 1);
    if (file_path)
    {
        const char* char_array = (*env)->GetStringUTFChars(env, string, NULL );
        memcpy(file_path, char_array, str_len);
        file_path[str_len] = '\0';
        log_info("[OpenFileCallback] file path: %s\n", file_path);

        lock_core();
            if (!mgb_load_rom_fd(fd, own_fd, file_path))
            {
                log_error("[OpenFileCallback] failed to open: %s\n", file_path);
            }
            else
            {
                log_info("[OpenFileCallback] opened: %s\n", file_path);
            }
        unlock_core();

        (*env)->ReleaseStringUTFChars(env, string, char_array);
        SDL_free(file_path);
    }

    log_info("[OpenFileCallback] finished!\n");
}

#endif // ANDROID

static uint32_t core_colour_callback(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b)
{
    UNUSED(user);
    uint32_t R = 0, G = 0, B = 0;

    switch (type)
    {
        case GB_ColourCallbackType_DMG:
            R = r;
            G = g;
            B = b;
            break;

        case GB_ColourCallbackType_GBC:
            #if 0
            R = (r * 7) + (r >> 3);
            G = (g * 7) + (g >> 3);
            B = (b * 7) + (b >> 3);
            #else
            // SOURCE: https://near.sh/articles/video/color-emulation
            R = (r * 26 + g *  4 + b *  2);
            G = (         g * 24 + b *  8);
            B = (r *  6 + g *  4 + b * 22);
            R = SDL_min(960, R) >> 2;
            G = SDL_min(960, G) >> 2;
            B = SDL_min(960, B) >> 2;
            #endif
            break;
    }

    return SDL_MapRGB(pixel_format, R, G, B);

}

static void core_vblank_callback(void* user)
{
    UNUSED(user);

    if (emu.speed > 0)
    {
        emu.gb.cycles_left_to_run = SDL_min(emu.gb.cycles_left_to_run, (GB_FRAME_CPU_CYCLES / 2) * emu.speed);
    }

    static int index = 0;

    index ^= 1;
    backbuffer = pixels_buffers[index];
    frontbuffer = pixels_buffers[index ^ 1];
    emu.pending_frame = true;
    GB_set_pixels(&emu.gb, backbuffer, GB_SCREEN_WIDTH, pixel_format->BytesPerPixel);
}

static void sdl2_display_event(const SDL_DisplayEvent* e)
{
    static const char* const orientation[] =
    {
        [SDL_ORIENTATION_UNKNOWN] = "UNKNOWN",
        [SDL_ORIENTATION_LANDSCAPE] = "LANDSCAPE",
        [SDL_ORIENTATION_LANDSCAPE_FLIPPED] = "LANDSCAPE_FLIPPED",
        [SDL_ORIENTATION_PORTRAIT] = "PORTRAIT",
        [SDL_ORIENTATION_PORTRAIT_FLIPPED] = "PORTRAIT_FLIPPED",
    };

    switch (e->event)
    {
        case SDL_DISPLAYEVENT_ORIENTATION:
            log_info("[DISPLAY-EVENT] changed display: %u orientation: %s\n", e->display, orientation[e->data1]);
            break;

#if SDL_VERSION_ATLEAST(2, 0, 12)
        case SDL_DISPLAYEVENT_CONNECTED:
            log_info("[DISPLAY-EVENT] Unhandled: SDL_DISPLAYEVENT_CONNECTED display: %u\n", e->display);
            break;

        case SDL_DISPLAYEVENT_DISCONNECTED:
            log_info("[DISPLAY-EVENT] Unhandled: SDL_DISPLAYEVENT_DISCONNECTED display: %u\n", e->display);
            break;
#endif
    }
}

static void sdl2_ctrl_key_event(const SDL_KeyboardEvent* e, bool down)
{
    if (down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_EQUALS:
            case SDL_SCANCODE_KP_PLUS:
                scale_screen(emu.scale + 1);
                break;

            case SDL_SCANCODE_MINUS:
            case SDL_SCANCODE_KP_PLUSMINUS:
            case SDL_SCANCODE_KP_MINUS:
                scale_screen(emu.scale - 1);
                break;

            case SDL_SCANCODE_1:
            case SDL_SCANCODE_2:
            case SDL_SCANCODE_3:
            case SDL_SCANCODE_4:
            case SDL_SCANCODE_5:
            case SDL_SCANCODE_6:
            case SDL_SCANCODE_7:
            case SDL_SCANCODE_8:
            case SDL_SCANCODE_9:
                set_speed((e->keysym.scancode - SDL_SCANCODE_1) + 1);
                break;

            case SDL_SCANCODE_F:
                toggle_fullscreen();
                break;

            case SDL_SCANCODE_L:
                loadstate(NULL);
                break;

            case SDL_SCANCODE_S:
                savestate(NULL);
                break;

            case SDL_SCANCODE_O:
                mgb_load_rom_filedialog();
                break;

            default: break; // silence enum warning
        }
    }
}

static void sdl2_ctrl_shift_key_event(const SDL_KeyboardEvent* e, bool down)
{
    if (down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_S:

                // mgb_save_state_filedialog();
                break;

            case SDL_SCANCODE_L:
                // mgb_load_state_filedialog();
                break;

            case SDL_SCANCODE_1:
            case SDL_SCANCODE_2:
            // lower values are way too slow
            // case SDL_SCANCODE_3:
            // case SDL_SCANCODE_4:
            // case SDL_SCANCODE_5:
            // case SDL_SCANCODE_6:
            // case SDL_SCANCODE_7:
            // case SDL_SCANCODE_8:
            // case SDL_SCANCODE_9:
                set_speed(-((e->keysym.scancode - SDL_SCANCODE_1) + 1));
                break;

            default: break; // silence enum warning
        }
    }
}

static void sdl2_key_event(const SDL_KeyboardEvent* e)
{
    const bool down = e->type == SDL_KEYDOWN;
    const bool ctrl = (e->keysym.mod & KMOD_CTRL) > 0;
    const bool shift = (e->keysym.mod & KMOD_SHIFT) > 0;

    if (ctrl)
    {
        if (shift)
        {
            sdl2_ctrl_shift_key_event(e, down);
        }
        else
        {
            sdl2_ctrl_key_event(e, down);
        }
        return;
    }

    if (!emu.running)
    {
        return;
    }

    switch (e->keysym.scancode)
    {
        case SDL_SCANCODE_X:        set_emu_button(GB_BUTTON_A, down);        break;
        case SDL_SCANCODE_Z:        set_emu_button(GB_BUTTON_B, down);        break;
        case SDL_SCANCODE_RETURN:   set_emu_button(GB_BUTTON_START, down);    break;
        case SDL_SCANCODE_SPACE:    set_emu_button(GB_BUTTON_SELECT, down);   break;
        case SDL_SCANCODE_UP:       set_emu_button(GB_BUTTON_UP, down);       break;
        case SDL_SCANCODE_DOWN:     set_emu_button(GB_BUTTON_DOWN, down);     break;
        case SDL_SCANCODE_LEFT:     set_emu_button(GB_BUTTON_LEFT, down);     break;
        case SDL_SCANCODE_RIGHT:    set_emu_button(GB_BUTTON_RIGHT, down);    break;

    #ifndef EMSCRIPTEN
        case SDL_SCANCODE_ESCAPE:
            emu.running = false;
            break;
    #endif // EMSCRIPTEN

        default: break; // silence enum warning
    }
}

static void sdl2_mouse_button_event(const SDL_MouseButtonEvent* e)
{
    // we already handle touch events...
    if (e->which == SDL_TOUCH_MOUSEID)
    {
        return;
    }

    switch (e->type)
    {
        case SDL_MOUSEBUTTONUP:
            on_mouse_up(e->which);
            break;

        case SDL_MOUSEBUTTONDOWN:
            on_mouse_down(e->which, e->x, e->y);
            break;
    }
}

static void sdl2_mouse_motion_event(const SDL_MouseMotionEvent* e)
{
    // we already handle touch events!
    if (e->which == SDL_TOUCH_MOUSEID)
    {
        return;
    }

    // only handle left clicks!
    if (e->state & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        on_mouse_motion(e->which, e->x, e->y);
    }
}

static void sdl2_generic_axis_event(int value, int axis)
{
    enum
    {
        deadzone = 8000,
        left     = -deadzone,
        right    = +deadzone,
        up       = -deadzone,
        down     = +deadzone,
    };

    switch (axis)
    {
        case SDL_CONTROLLER_AXIS_LEFTX: case SDL_CONTROLLER_AXIS_RIGHTX:
            if (value < left)
            {
                set_emu_button(GB_BUTTON_LEFT, true);
                set_emu_button(GB_BUTTON_RIGHT, false);
            }
            else if (value > right)
            {
                set_emu_button(GB_BUTTON_LEFT, false);
                set_emu_button(GB_BUTTON_RIGHT, true);
            }
            else
            {
                set_emu_button(GB_BUTTON_LEFT, false);
                set_emu_button(GB_BUTTON_RIGHT, false);
            }
            break;

        case SDL_CONTROLLER_AXIS_LEFTY: case SDL_CONTROLLER_AXIS_RIGHTY:
            if (value < up)
            {
                set_emu_button(GB_BUTTON_UP, true);
                set_emu_button(GB_BUTTON_DOWN, false);
            }
            else if (value > down)
            {
                set_emu_button(GB_BUTTON_UP, false);
                set_emu_button(GB_BUTTON_DOWN, true);
            }
            else
            {
                set_emu_button(GB_BUTTON_UP, false);
                set_emu_button(GB_BUTTON_DOWN, false);
            }
            break;
    }
}

static void sdl2_controller_axis_event(const SDL_ControllerAxisEvent* e)
{
    sdl2_generic_axis_event(e->value, e->axis);
}

void set_emu_button(uint8_t gb_button, bool down)
{
    lock_core();
        GB_set_buttons(&emu.gb, gb_button, down);
    unlock_core();
}

static void sdl2_joy_axis_event(const SDL_JoyAxisEvent* e)
{
    #ifdef __SWITCH__
        sdl2_generic_axis_event(e->value, e->axis);
    #else
        UNUSED(e);
    #endif
}

static void sdl2_joy_event(const SDL_JoyButtonEvent* e)
{
    #ifndef __SWITCH__
        UNUSED(e);
        return;
    #endif

    enum
    {
        JOY_A = 0,
        JOY_B = 1,
        JOY_X = 2,
        JOY_Y = 3,
        JOY_L3 = 4,
        JOY_R3 = 5,
        JOY_L = 6,
        JOY_R = 7,
        JOY_L2 = 8,
        JOY_R2 = 9,
        JOY_START = 10,
        JOY_BACK = 11,

        JOY_DPAD_LEFT = 12,
        JOY_DPAD_UP = 13,
        JOY_DPAD_RIGHT = 14,
        JOY_DPAD_DOWN = 15,

        JOY_LSTICK_LEFT = 16,
        JOY_LSTICK_UP = 17,
        JOY_LSTICK_RIGHT = 18,
        JOY_LSTICK_DOWN = 19,

        JOY_RSTICK_LEFT = 20,
        JOY_RSTICK_UP = 21,
        JOY_RSTICK_RIGHT = 22,
        JOY_RSTICK_DOWN = 23,
    };

    log_info("joy button: %u\n", e->button);

    const bool down = e->type == SDL_JOYBUTTONDOWN;

    switch (e->button)
    {
        case JOY_A:               set_emu_button(GB_BUTTON_A, down);         break;
        case JOY_B:               set_emu_button(GB_BUTTON_B, down);         break;
        case JOY_X:               break; // todo, open menu
        case JOY_Y:               break;
        case JOY_START:           set_emu_button(GB_BUTTON_START, down);     break;
        case JOY_BACK:            set_emu_button(GB_BUTTON_SELECT, down);    break;
        case JOY_DPAD_UP:         set_emu_button(GB_BUTTON_UP, down);        break;
        case JOY_DPAD_DOWN:       set_emu_button(GB_BUTTON_DOWN, down);      break;
        case JOY_DPAD_LEFT:       set_emu_button(GB_BUTTON_LEFT, down);      break;
        case JOY_DPAD_RIGHT:      set_emu_button(GB_BUTTON_RIGHT, down);     break;
    }
}

static void sdl2_controller_event(const SDL_ControllerButtonEvent* e)
{
    const bool down = e->type == SDL_CONTROLLERBUTTONDOWN;

    switch (e->button)
    {
        case SDL_CONTROLLER_BUTTON_A:               set_emu_button(GB_BUTTON_A, down);         break;
        case SDL_CONTROLLER_BUTTON_B:               set_emu_button(GB_BUTTON_B, down);         break;
        case SDL_CONTROLLER_BUTTON_X:               break;
        case SDL_CONTROLLER_BUTTON_Y:               break;
        case SDL_CONTROLLER_BUTTON_START:           set_emu_button(GB_BUTTON_START, down);     break;
        case SDL_CONTROLLER_BUTTON_BACK:            set_emu_button(GB_BUTTON_SELECT, down);    break;
        case SDL_CONTROLLER_BUTTON_GUIDE:           break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:    break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:       break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:   break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:      break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:         set_emu_button(GB_BUTTON_UP, down);        break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:       set_emu_button(GB_BUTTON_DOWN, down);      break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:       set_emu_button(GB_BUTTON_LEFT, down);      break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:      set_emu_button(GB_BUTTON_RIGHT, down);     break;
    }
}

static void sdl2_controller_device_event(const SDL_ControllerDeviceEvent* e)
{
    switch (e->type)
    {
        case SDL_CONTROLLERDEVICEADDED:
            if (game_controller)
            {
                SDL_GameControllerClose(game_controller);
            }
            game_controller = SDL_GameControllerOpen(e->which);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            if (game_controller)
            {
                SDL_GameControllerClose(game_controller);
                game_controller = NULL;
            }
            break;
    }
}

static void sdl2_touch_event(const SDL_TouchFingerEvent* e)
{
    int win_w = 0, win_h = 0;

    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    // we need to un-normalise x, y
    const int x = e->x * win_w;
    const int y = e->y * win_h;

    switch (e->type)
    {
        case SDL_FINGERUP:
            on_touch_up(e->fingerId);
            break;

        case SDL_FINGERDOWN:
            on_touch_down(e->fingerId, x, y);
            break;

        case SDL_FINGERMOTION:
            on_touch_motion(e->fingerId, x, y);
            break;
    }
}

static void sdl2_window_event(const SDL_WindowEvent* e)
{
    switch (e->event)
    {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_HIDDEN:
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_MOVED:
        case SDL_WINDOWEVENT_RESIZED:
            break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            // use this rather than window size because i had issues with hi-dpi screens.
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);
            on_resize(w, h);
        }   break;

        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_LEAVE:
        case SDL_WINDOWEVENT_FOCUS_GAINED:
        case SDL_WINDOWEVENT_FOCUS_LOST:
        case SDL_WINDOWEVENT_CLOSE:
        case SDL_WINDOWEVENT_TAKE_FOCUS:
        case SDL_WINDOWEVENT_HIT_TEST:
            break;
    }
}

void savestate(const char* path)
{
    lock_core();
        mgb_save_state_file(path);
    unlock_core();
}

void loadstate(const char* path)
{
    lock_core();
        mgb_load_state_file(path);
    unlock_core();
}

static void setup_rect(int w, int h)
{
    const int min_scale = get_scale(w, h);

    rect.w = WIDTH * min_scale;
    rect.h = HEIGHT * min_scale;
    rect.x = (w - rect.w) / 2;
    rect.y = (h - rect.h) / 2;
}

static void on_resize(int w, int h)
{
    log_info("on resize called. w: %d h: %d\n", w, h);
    setup_rect(w, h);
    on_touch_resize(w, h);
}

void set_speed(int x)
{
    // audio_get_core_sample_rate
    emu.speed = x;
    lock_core();
        audio_update_core_sample_rate(&emu);
    unlock_core();
}

int get_scale(int w, int h)
{
    const int scale_w = w / WIDTH;
    const int scale_h = h / HEIGHT;

    return SDL_min(scale_w, scale_h);
}

void scale_screen(int new_scale)
{
    emu.scale = SDL_max(1, new_scale);
    SDL_SetWindowSize(window, WIDTH * emu.scale, HEIGHT * emu.scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

bool is_fullscreen(void)
{
    return (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) > 0;
}

void set_fullscreen(bool enable)
{
    int flags = 0; // disable fullscreen

    if (enable)
    {
        flags |= SDL_WINDOW_FULLSCREEN; // enable fullscreen
    }

    SDL_SetWindowFullscreen(window, flags);
}

void toggle_fullscreen(void)
{
    set_fullscreen(is_fullscreen() ^ 1);
}

void toggle_vsync(void)
{
    emu.vsync ^= 1;
    audio_lock();
        audio_get_elapsed_cycles();
    audio_unlock();
#ifdef ANDROID
    const char* str = emu.vsync ? "Enabled Vsync: This may cause audio to go out of sync!" : "Disabled Vsync";
    SDL_AndroidShowToast(str, 0, -1, 0, 0);
#endif
}

static void run(double delta)
{
    const time_t the_time = time(NULL);
    const struct tm* tm = localtime(&the_time);

    if (!tm)
    {
        log_error("failed to set rtc\n");
        return;
    }

    const struct GB_Rtc rtc =
    {
        .S = tm->tm_sec,
        .M = tm->tm_min,
        .H = tm->tm_hour,
        .DL = tm->tm_yday & 0xFF,
        .DH = tm->tm_yday > 0xFF,
    };

    if (mgb_has_rom() && emu.running && get_menu_type() == MenuType_ROM)
    {
        // this is unused atm.
        if (emu.rewinding)
        {
            lock_core();
                const int speed = emu.speed > 1 ? emu.speed : 1;

                for (int i = 0; i < speed; i++)
                {
                    if (mgb_rewind_pop_frame(frontbuffer, pixel_format->BytesPerPixel * HEIGHT * WIDTH))
                    {
                        emu.pending_frame = true;
                    }
                }
            unlock_core();
        }
        else if (emu.vsync)
        {
            GB_set_rtc(&emu.gb, rtc);

            // just in case something sends the main thread to sleep
            // ie, filedialog, then cap the max delta to something reasonable!
            // maybe keep track of deltas here to get an average?
            delta = SDL_min(delta, 1.333333);

            audio_lock();
                uint32_t audio_cycles = audio_get_elapsed_cycles();
                audio_cycles = SDL_min(audio_cycles, 1000);
                double cycles = delta * (GB_FRAME_CPU_CYCLES - audio_cycles);
            audio_unlock();

            {
                static int counter = 0;
                counter++;
                if (counter == 60)
                {
                    // log_info("running for cycles: %d NORMAL: %d diff: %d\n", (int)cycles, GB_FRAME_CPU_CYCLES, (int)cycles - GB_FRAME_CPU_CYCLES);
                    counter = 0;
                }
            }

            if (emu.speed > 1)
            {
                cycles *= emu.speed;
            }
            else if (emu.speed < -1)
            {
                cycles /= emu.speed * -1;
            }

            if (emu.pending_frame && emu.speed > 0)
            {
                cycles = SDL_min(cycles, (GB_FRAME_CPU_CYCLES - 2000) * emu.speed);
                // update_game_texture();
            }

            lock_core();
                GB_run(&emu.gb, cycles);
            unlock_core();
        }
    }
}

static void update_game_texture(void)
{
    if (mgb_has_rom())
    {
        lock_core();
            if (emu.pending_frame && !emu.rendered_frame)
            {
                void* pixels = NULL; int pitch = 0;

                SDL_LockTexture(texture, NULL, &pixels, &pitch);
                    SDL_memcpy(pixels, frontbuffer, pixel_format->BytesPerPixel * HEIGHT * WIDTH);
                SDL_UnlockTexture(texture);

                emu.pending_frame = false;
                emu.rendered_frame = true;
            }
        unlock_core();
    }
}

static void render(void)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    update_game_texture();
    if (mgb_has_rom())
    {
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
    emu.rendered_frame = false;

    touch_render(renderer);

    SDL_RenderPresent(renderer);
}

static void events(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                emu.running = false;
                log_info("[EVENT] Unhandled: SDL_QUIT\n");
                return;

            case SDL_APP_TERMINATING:
                log_info("[EVENT] Unhandled: SDL_APP_TERMINATING\n");
                break;

            case SDL_APP_LOWMEMORY:
                log_info("[EVENT] Unhandled: SDL_APP_LOWMEMORY\n");
                break;

            case SDL_APP_WILLENTERBACKGROUND:
                mgb_save_save_file(NULL);
                log_info("[EVENT] Unhandled: SDL_APP_WILLENTERBACKGROUND\n");
                break;

            case SDL_APP_DIDENTERBACKGROUND:
                log_info("[EVENT] Unhandled: SDL_APP_DIDENTERBACKGROUND\n");
                break;

            case SDL_APP_WILLENTERFOREGROUND:
            #ifdef ANDROID
                // hack to fix sdl bug where the screen is no longer fullscreen
                // but sdl still thinks it is...
                // set_fullscreen(false);
                // set_fullscreen(true);
            #endif
                log_info("[EVENT] Unhandled: SDL_APP_WILLENTERFOREGROUND\n");
                break;

            case SDL_APP_DIDENTERFOREGROUND:
                // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "error", "hello world!", window);
                // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "warn", "hello world!", window);
                // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "info", "hello world!", window);
                #if defined(ANDROID)
                    set_fullscreen(false);
                    set_fullscreen(true);
                #endif
                log_info("[EVENT] Unhandled: SDL_APP_DIDENTERFOREGROUND\n");
                break;

#if SDL_VERSION_ATLEAST(2, 0, 12)
            case SDL_LOCALECHANGED:
                log_info("[EVENT] Unhandled: SDL_LOCALECHANGED\n");
                break;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 9)
            case SDL_DISPLAYEVENT:
                log_info("display event\n");
                sdl2_display_event(&e.display);
                break;
#endif

            case SDL_WINDOWEVENT:
                log_info("window event\n");
                sdl2_window_event(&e.window);
                break;

            case SDL_SYSWMEVENT:
                log_info("[EVENT] Unhandled: SDL_SYSWMEVENT\n");
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                sdl2_key_event(&e.key);
                break;

            case SDL_TEXTEDITING:
            case SDL_TEXTINPUT:
                break;

#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_KEYMAPCHANGED:
                break;
#endif

            case SDL_MOUSEMOTION:
                sdl2_mouse_motion_event(&e.motion);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                sdl2_mouse_button_event(&e.button);
                break;

            case SDL_MOUSEWHEEL:
                log_info("[EVENT] Unhandled: SDL_MOUSEWHEEL\n");
                break;

            case SDL_JOYAXISMOTION:
                sdl2_joy_axis_event(&e.jaxis);
                break;

            case SDL_JOYBALLMOTION:
                log_info("[EVENT] Unhandled: SDL_JOYBALLMOTION\n");
                break;

            case SDL_JOYHATMOTION:
                log_info("[EVENT] Unhandled: SDL_JOYHATMOTION\n");
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                sdl2_joy_event(&e.jbutton);
                break;

            case SDL_JOYDEVICEADDED:
                log_info("[EVENT] Unhandled: SDL_JOYDEVICEADDED\n");
                break;

            case SDL_JOYDEVICEREMOVED:
                log_info("[EVENT] Unhandled: SDL_JOYDEVICEREMOVED\n");
                break;

            case SDL_CONTROLLERAXISMOTION:
                sdl2_controller_axis_event(&e.caxis);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                sdl2_controller_event(&e.cbutton);
                break;

            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                sdl2_controller_device_event(&e.cdevice);
                break;

#if SDL_VERSION_ATLEAST(2, 0, 12)
            case SDL_CONTROLLERTOUCHPADDOWN:
            case SDL_CONTROLLERTOUCHPADMOTION:
            case SDL_CONTROLLERTOUCHPADUP:
            case SDL_CONTROLLERSENSORUPDATE:
                break;
#endif

            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                sdl2_touch_event(&e.tfinger);
                break;

            case SDL_DOLLARGESTURE:
            case SDL_DOLLARRECORD:
            case SDL_MULTIGESTURE:
            case SDL_CLIPBOARDUPDATE:
                break;

            case SDL_DROPFILE:
                if (romloader_open(e.drop.file))
                {
                    log_info("[DROP-EVENT] !!!!!!!!!! opened drop event: %s\n", e.drop.file);
                }
                else
                {
                    log_info("[DROP-EVENT] failed to open drop event: %s\n", e.drop.file);
                }
                SDL_free(e.drop.file);
                break;

            case SDL_DROPTEXT:
                break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_DROPBEGIN:
                log_info("[EVENT] Unhandled: SDL_DROPBEGIN\n");
                break;

            case SDL_DROPCOMPLETE:
                log_info("[EVENT] Unhandled: SDL_DROPCOMPLETE\n");
                break;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_AUDIODEVICEADDED:
                log_info("[EVENT] Unhandled: SDL_AUDIODEVICEADDED\n");
                break;

            case SDL_AUDIODEVICEREMOVED:
                log_info("[EVENT] Unhandled: SDL_AUDIODEVICEREMOVED\n");
                break;
#endif

            case SDL_SENSORUPDATE:
                log_info("[EVENT] Unhandled: SDL_SENSORUPDATE\n");
                break;

#if SDL_VERSION_ATLEAST(2, 0, 2)
            case SDL_RENDER_TARGETS_RESET:
                log_info("[EVENT] Unhandled: SDL_RENDER_TARGETS_RESET\n");
                break;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_RENDER_DEVICE_RESET:
                log_info("[EVENT] Unhandled: SDL_RENDER_DEVICE_RESET\n");
                break;
#endif
        }
    }
}

static void cleanup(void)
{
    log_info("begin exit\n");

    // de-allocate sdl first, then core etc
    touch_exit();
    audio_exit();
    if (game_controller)    { SDL_GameControllerClose(game_controller); }
    if (prev_texture)       { SDL_DestroyTexture(prev_texture); }
    if (texture)            { SDL_DestroyTexture(texture); }
    if (renderer)           { SDL_DestroyRenderer(renderer); }
    if (window)             { SDL_DestroyWindow(window); }
    if (mutex)              { SDL_DestroyMutex(mutex); }
    if (pixel_format)       { SDL_free(pixel_format); }
    if (pixels_buffers[0])  { SDL_free(pixels_buffers[0]); }
    if (pixels_buffers[1])  { SDL_free(pixels_buffers[1]); }

    mgb_exit();

    log_info("exiting...\n");
    SDL_Quit();
}

static void syncfs(void)
{
    #ifdef EMSCRIPTEN
    EM_ASM(
        FS.syncfs(false, function (err) {
            if (err) {
                console.log(err);
            }
        });
    );
    #endif // EMSCRIPTEN
}

#ifdef EMSCRIPTEN
static void flushsave(void)
{
    // sync every 60ticks
    static int counter = 0;
    counter++;

    if (counter < 60)
    {
        return;
    }
    counter = 0;

    lock_core();
        mgb_save_save_file(NULL);
    unlock_core();
}

EMSCRIPTEN_KEEPALIVE void em_load_rom_data(const char* name, const uint8_t* data, int len)
{
    log_info("[EM] loading rom! name: %s len: %d\n", name, len);

    if (len <= 0)
    {
        log_info("[EM] invalid rom size!\n");
        return;
    }

    if (mgb_load_rom_data(name, data, len))
    {
        EM_ASM({
            let button = document.getElementById('HackyButton');
            button.style.visibility = "hidden";
        });
        log_info("[EM] loaded rom! name: %s len: %d\n", name, len);
    }
}

static void em_loop(void)
{
    static Uint64 start = 0;
    static Uint64 now = 0;

    static const double ms = 1000.0;
    static const double div_60 = ms / 60;
    static double delta = div_60;

    if (start == 0)
    {
        start = SDL_GetPerformanceCounter();
    }

    events();
    run(delta / div_60);
    render();

    flushsave();

    now = SDL_GetPerformanceCounter();
    delta = (double)((now - start)*1000.0) / SDL_GetPerformanceFrequency();
    start = now;
}
#endif // #ifdef EMSCRIPTEN

void change_menu(enum MenuType new_menu)
{
    on_touch_menu_change(new_menu);
    emu.menu_type = new_menu;
}

enum MenuType get_menu_type(void)
{
    return emu.menu_type;
}

void lock_core(void)
{
    SDL_LockMutex(mutex);
}

void unlock_core(void)
{
    SDL_UnlockMutex(mutex);
}

static void on_file_callback(const char* file, enum CallbackType type, bool success)
{
    const char* str = NULL;

    switch (type)
    {
        case CallbackType_LOAD_ROM:
            str = success ? "Loaded rom" : "Failed to load rom";
            break;
        case CallbackType_LOAD_SAVE:
            str = success ? "Loaded save" : NULL;
            break;
        case CallbackType_LOAD_STATE:
            str = success ? "Loaded state" : "Failed to load state";
            break;
        case CallbackType_SAVE_SAVE:
            if (success)
            {
                syncfs();
            }
            break;
        case CallbackType_SAVE_STATE:
            str = success ? "Saved state" : "Failed to save state";
            if (success)
            {
                syncfs();
            }
            break;
    }

    if (str)
    {
        #ifdef ANDROID
            SDL_AndroidShowToast(str, 0, -1, 0, 0);
        #endif
    }
}

void on_title_click(void)
{
    #ifdef ANDROID
        SDL_AndroidShowToast(ss_build(
            "%s\nSDL compiled version: %d.%d.%d\nSDL linked version: %d.%d.%d\nAndroid SDK: %d\nVideo driver: %s\nWindow format: %s\nDisplay format: %s\nRefresh rate: %d\nAudio driver: %s\nAudio format: %d\nAudio freq: %d\nAudio channels: %d\nAudio samples: %d\nAudio size: %d\nAudio silence: %d\nCPU count: %d\nCPU cache line size: %d\nSytem ram: %d MiB\nAndroid TV: %d\nChrombook: %d\nDexMode: %d\nTablet: %d",
            VERSION_STRING,
            version_compiled.major, version_compiled.minor, version_compiled.patch,
            version_linked.major, version_linked.minor, version_linked.patch,
            SDL_GetAndroidSDKVersion(),
            SDL_GetCurrentVideoDriver(),
            SDL_GetPixelFormatName(pixel_format_enum),
            SDL_GetPixelFormatName(display_mode.format),
            display_mode.refresh_rate,
            SDL_GetCurrentAudioDriver(),
            audio_get_spec().format,
            audio_get_spec().freq,
            audio_get_spec().channels,
            audio_get_spec().samples,
            audio_get_spec().size,
            audio_get_spec().silence,
            SDL_GetCPUCount(),
            SDL_GetCPUCacheLineSize(),
            SDL_GetSystemRAM(),
            SDL_IsAndroidTV(),
            SDL_IsChromebook(),
            SDL_IsDeXMode(),
            SDL_IsTablet()
            ).str, 1, -1, 0, 0
        );
    #else
    #endif
}

#ifdef __SWITCH__
#define THROW_IF(func) do { Result r = func; if (R_FAILED(r)) fatalThrow(r); } while(0)

#ifndef NDEBUG
    #include <unistd.h> // for close(socket);
    static int nxlink_socket = 0;
#endif

// all services that i need are init here.
// no services are to be init later on in the code.
void userAppInit(void) {
    THROW_IF(appletLockExit());
    THROW_IF(socketInitializeDefault());
    THROW_IF(romfsInit());
#ifndef NDEBUG
    nxlink_socket = nxlinkStdio();
#endif // NDEBUG
}

void userAppExit(void) {
#ifndef NDEBUG
#ifdef __SWITCH__
    close(nxlink_socket);
#endif // __SWITCH__
#endif // NDEBUG
    romfsExit();
    socketExit();
    appletUnlockExit();
}
#endif


int main(int argc, char* argv[])
{
    emu.scale = DEFAULT_SCALE;
    emu.speed = DEFAULT_SPEED;
    emu.vsync = DEFAULT_SYNC_VSYNC;

    if (!GB_init(&emu.gb))
    {
        goto fail;
    }

    SDL_VERSION(&version_compiled);
    SDL_GetVersion(&version_linked);
    log_info("[SDL2-VERSION] compiled: %d.%d.%d revision: %s\n", version_compiled.major, version_compiled.minor, version_compiled.patch, SDL_GetRevision());
    log_info("[SDL2-VERSION] linked: %d.%d.%d revision: %s\n", version_linked.major, version_linked.minor, version_linked.patch, SDL_GetRevision());
    log_info("\n");

    if (SDL_Init(DEFAULT_SDL_INIT_FLAGS) != 0)
    {
        log_error("failed to init sdl\n");
        goto fail;
    }

    #ifdef __SWITCH__
        // for some reason the controller api isn't implemented...
        SDL_JoystickOpen(0);
    #endif

    // for web / android, fill the screen
    int screenw = WIDTH * emu.scale, screenh = HEIGHT * emu.scale;
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
#if defined(EMSCRIPTEN) || defined(ANDROID) || defined(__SWITCH__)
    SDL_DisplayMode display = {0};
    SDL_GetCurrentDisplayMode(0, &display);
    screenw = display.w;
    screenh = display.h;
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

    window = SDL_CreateWindow("TotalGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenw, screenh, flags);
    if (!window)
    {
        goto fail;
    }

    for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
    {
        SDL_GetCurrentDisplayMode(i, &display_mode);

        log_info("\t[SDL-DISPLAY_MODE %d] format: %s\n", i, SDL_GetPixelFormatName(display_mode.format));
        log_info("\t[SDL-DISPLAY_MODE %d] w: %d\n", i, display_mode.w);
        log_info("\t[SDL-DISPLAY_MODE %d] h: %d\n", i, display_mode.h);
        log_info("\t[SDL-DISPLAY_MODE %d] refresh_rate: %dhz\n", i, display_mode.refresh_rate);
        log_info("\n");
    }

    int nJoysticks = SDL_NumJoysticks();
    int nGameControllers = 0;
    for (int i = 0; i < nJoysticks; i++) {
        if (SDL_IsGameController(i)) {
            nGameControllers++;
        }
    }

    log_info("num of joysticks: %d num of controllers: %d\n", nJoysticks, nGameControllers);

    if (SDL_GetCurrentDisplayMode(0, &display_mode))
    {
        goto fail;
    }

    pixel_format_enum = SDL_GetWindowPixelFormat(window);
    pixel_format = SDL_AllocFormat(pixel_format_enum);
    if (!pixel_format)
    {
        goto fail;
    }

    log_info("[SDL-WINDOW_FORMAT] format: %s\n", SDL_GetPixelFormatName(pixel_format_enum));
    log_info("[SDL-WINDOW_FORMAT] BitsPerPixel: %u\n", pixel_format->BitsPerPixel);
    log_info("[SDL-WINDOW_FORMAT] BytesPerPixel: %u\n", pixel_format->BytesPerPixel);
    log_info("[SDL-WINDOW_FORMAT] Rmask: 0x%X\n", pixel_format->Rmask);
    log_info("[SDL-WINDOW_FORMAT] Gmask: 0x%X\n", pixel_format->Gmask);
    log_info("[SDL-WINDOW_FORMAT] Bmask: 0x%X\n", pixel_format->Bmask);
    log_info("[SDL-WINDOW_FORMAT] Amask: 0x%X\n", pixel_format->Amask);
    log_info("[SDL-WINDOW_FORMAT] Rloss: %u\n", pixel_format->Rloss);
    log_info("[SDL-WINDOW_FORMAT] Gloss: %u\n", pixel_format->Gloss);
    log_info("[SDL-WINDOW_FORMAT] Bloss: %u\n", pixel_format->Bloss);
    log_info("[SDL-WINDOW_FORMAT] Aloss: %u\n", pixel_format->Aloss);
    log_info("\n");

    pixels_buffers[0] = SDL_calloc(pixel_format->BytesPerPixel, HEIGHT * WIDTH);
    pixels_buffers[1] = SDL_calloc(pixel_format->BytesPerPixel, HEIGHT * WIDTH);
    if (!pixels_buffers[0] || !pixels_buffers[1])
    {
        goto fail;
    }

    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    if (!renderer)
    {
        goto fail;
    }

    if (SDL_GetRendererInfo(renderer, &renderer_info))
    {
        goto fail;
    }

    log_info("[SDL-RENDERER_INFO] name: %s\n", renderer_info.name);
    log_info("[SDL-RENDERER_INFO] flags: 0x%X\n", renderer_info.flags);
    log_info("[SDL-RENDERER_INFO] num_texture_formats: %u\n", renderer_info.num_texture_formats);
    for (uint32_t i = 0; i < renderer_info.num_texture_formats; i++)
    {
        log_info("\t[SDL-RENDERER_INFO] texture_fromat %u: %s\n", i, SDL_GetPixelFormatName(renderer_info.texture_formats[i]));
    }
    log_info("[SDL-RENDERER_INFO] max_texture_width: %u\n", renderer_info.max_texture_width);
    log_info("[SDL-RENDERER_INFO] max_texture_height: %u\n", renderer_info.max_texture_height);
    log_info("\n");

    // try and enable blending
    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND))
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    prev_texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture || !prev_texture)
    {
        goto fail;
    }

    mutex = SDL_CreateMutex();
    if (!mutex)
    {
        goto fail;
    }

    if (!audio_init(&emu))
    {
        goto fail;
    }

    if (!touch_init(renderer))
    {
        goto fail;
    }

    static const char* const power_states[] =
    {
        [SDL_POWERSTATE_UNKNOWN] = "UNKNOWN",
        [SDL_POWERSTATE_ON_BATTERY] = "ON_BATTERY",
        [SDL_POWERSTATE_NO_BATTERY] = "NO_BATTERY",
        [SDL_POWERSTATE_CHARGING] = "CHARGING",
        [SDL_POWERSTATE_CHARGED] = "CHARGED",
    };

    // this fails on some os, like chromeos
    SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);
    on_resize(screenw, screenh);

    backbuffer = pixels_buffers[0];
    frontbuffer = pixels_buffers[1];

    GB_set_vblank_callback(&emu.gb, core_vblank_callback, &emu);
    GB_set_colour_callback(&emu.gb, core_colour_callback, &emu);
    GB_set_pixels(&emu.gb, backbuffer, GB_SCREEN_WIDTH, pixel_format->BytesPerPixel);
    GB_set_rtc_update_config(&emu.gb, GB_RTC_UPDATE_CONFIG_NONE);

    mgb_init(&emu.gb);
    mgb_set_on_file_callback(on_file_callback);
    emu.running = true;
    log_info("did mgb init\n");

    char* base_path = SDL_GetBasePath();
    char* pref_path = SDL_GetPrefPath(NULL, "TotalGB");
    log_info("got base path\n");

    log_info("[SDL-INFO] SDL_GetPlatform: %s\n", SDL_GetPlatform());
    log_info("[SDL-INFO] SDL_BYTEORDER: %s\n", SDL_BYTEORDER == SDL_LIL_ENDIAN ? "SDL_LIL_ENDIAN" : "SDL_BIG_ENDIAN");
    log_info("[SDL-INFO] SDL_GetCPUCount: %d\n", SDL_GetCPUCount());
    log_info("[SDL-INFO] SDL_GetCPUCacheLineSize: %d\n", SDL_GetCPUCacheLineSize());
    log_info("[SDL-INFO] SDL_HasRDTSC: %d\n", SDL_HasRDTSC());
    log_info("[SDL-INFO] SDL_HasAltiVec: %d\n", SDL_HasAltiVec());
    log_info("[SDL-INFO] SDL_HasMMX: %d\n", SDL_HasMMX());
    log_info("[SDL-INFO] SDL_Has3DNow: %d\n", SDL_Has3DNow());
    log_info("[SDL-INFO] SDL_HasSSE: %d\n", SDL_HasSSE());
    log_info("[SDL-INFO] SDL_HasSSE2: %d\n", SDL_HasSSE2());
    log_info("[SDL-INFO] SDL_HasSSE3: %d\n", SDL_HasSSE3());
    log_info("[SDL-INFO] SDL_HasSSE41: %d\n", SDL_HasSSE41());
    log_info("[SDL-INFO] SDL_HasSSE42: %d\n", SDL_HasSSE42());
    log_info("[SDL-INFO] SDL_HasAVX: %d\n", SDL_HasAVX());
    log_info("[SDL-INFO] SDL_HasAVX2: %d\n", SDL_HasAVX2());
    log_info("[SDL-INFO] SDL_HasAVX512F: %d\n", SDL_HasAVX512F());
    log_info("[SDL-INFO] SDL_HasARMSIMD: %d\n", SDL_HasARMSIMD());
    log_info("[SDL-INFO] SDL_HasNEON: %d\n", SDL_HasNEON());
    log_info("[SDL-INFO] SDL_GetSystemRAM: %d MiB %.1f GiB\n", SDL_GetSystemRAM(), SDL_GetSystemRAM() / 1024.0);
    log_info("[SDL-INFO] SDL_SIMDGetAlignment: %zu\n", SDL_SIMDGetAlignment());
    log_info("[SDL-INFO] SDL_GetCurrentVideoDriver: %s\n", SDL_GetCurrentVideoDriver());
    log_info("\n");

#if defined(ANDROID)
    // SDL_AndroidGetJNIEnv()
    // SDL_AndroidGetActivity()
    // SDL_AndroidBackButton()
    // SDL_AndroidGetExternalStorageState()
    // SDL_AndroidGetExternalStoragePath()
    // SDL_AndroidRequestPermission()
    // SDL_AndroidRequestPermission("android.permission.WRITE_EXTERNAL_STORAGE");

    log_info("[SDL_ANDROID-INFO] sdk version: %d\n", SDL_GetAndroidSDKVersion());
    log_info("[SDL_ANDROID-INFO] internal storage path: %s\n", SDL_AndroidGetInternalStoragePath());
    log_info("[SDL_ANDROID-INFO] external storage path: %s\n", SDL_AndroidGetExternalStoragePath());
    log_info("[SDL_ANDROID-INFO] external storage state: 0x%X\n", SDL_AndroidGetExternalStorageState());
    log_info("[SDL_ANDROID-INFO] SDL_IsAndroidTV(): %u\n", SDL_IsAndroidTV());
    log_info("[SDL_ANDROID-INFO] SDL_IsChromebook(): %u\n", SDL_IsChromebook());
    log_info("[SDL_ANDROID-INFO] SDL_IsDeXMode(): %u\n", SDL_IsDeXMode());
    log_info("[SDL_ANDROID-INFO] SDL_IsTablet(): %u\n", SDL_IsTablet());
    log_info("\n");

    SDL_AndroidShowToast(VERSION_STRING, 0, -1, 0, 0);
#endif

    if (base_path)
    {
        log_info("[SDL] SDL_GetBasePath: %s\n", base_path);
        SDL_free(base_path);
    }
    if (pref_path)
    {
        #if !defined(ANDROID) && !defined(EMSCRIPTEN)
            const struct SafeString ss_save = util_append_string(pref_path, "saves");
            const struct SafeString ss_state = util_append_string(pref_path, "states");
            const struct SafeString ss_rtc = util_append_string(pref_path, "rtc");

            mgb_set_save_folder(ss_save.str);
            mgb_set_state_folder(ss_state.str);
            mgb_set_rtc_folder(ss_rtc.str);
        #endif
        log_info("[SDL] SDL_GetPrefPath: %s\n", pref_path);
        SDL_free(pref_path);
    }

    int psecs, perc;
    SDL_PowerState power_state = SDL_GetPowerInfo(&psecs, &perc);
    log_info("[SDL-POWER] secs: %d percentage: %d state: %s\n", psecs, perc, power_states[power_state]);
    log_info("\n");

    for (int i = 0; i < SDL_NumSensors(); i++)
    {
        log_info("[SDL-SENSOR] %s\n", SDL_SensorGetDeviceName(i));
    }
    log_info("\n");

#if !defined(EMSCRIPTEN) && !defined(ANDROID)
    if (argc > 2)
    {
        if (!mgb_load_rom_file(argv[1]))
        {
            goto fail;
        }
    }
    #ifdef __SWITCH__
    const char* r = "/roms/gb/Legend of Zelda, The - Link's Awakening DX (USA, Europe) (SGB Enhanced).zip";
    if (!mgb_load_rom_file(r))
    {
        goto fail;
    }
    #endif
#elif defined(ANDROID)
    const struct SafeString ss_save = util_append_string(SDL_AndroidGetExternalStoragePath(), "/saves");
    const struct SafeString ss_state = util_append_string(SDL_AndroidGetExternalStoragePath(), "/states");
    const struct SafeString ss_rtc = util_append_string(SDL_AndroidGetExternalStoragePath(), "/rtc");

    mgb_set_save_folder(ss_save.str);
    mgb_set_state_folder(ss_state.str);
    mgb_set_rtc_folder(ss_rtc.str);
#elif defined(EMSCRIPTEN)
    mgb_set_save_folder("/saves");
    mgb_set_state_folder("/states");
    mgb_set_rtc_folder("/rtc");

    EM_ASM(
        if (!FS.analyzePath("/saves").exists) {
            FS.mkdir("/saves");
        }
        if (!FS.analyzePath("/states").exists) {
            FS.mkdir("/states");
        }
        if (!FS.analyzePath("/rtc").exists) {
            FS.mkdir("/rtc");
        }

        FS.mount(IDBFS, {}, "/saves");
        FS.mount(IDBFS, {}, "/states");
        FS.mount(IDBFS, {}, "/rtc");

        FS.syncfs(true, function (err) {
            if (err) {
                console.log(err);
            }
        });

        if (IsMobileBrowser()) {
            console.log("is a mobile browser");
            // _em_set_browser_type(true);
        } else {
            console.log("is NOT a mobile browser");
            // _em_set_browser_type(false);
        }
    );

    log_info("setting main loop\n");
    emscripten_set_main_loop(em_loop, 0, true);
#endif

    Uint64 start = SDL_GetPerformanceCounter();
    Uint64 now = 0;

    const double ms = 1000.0;
    const double div_60 = ms / 60;
    double delta = div_60;

    while (emu.running)
    {
        events();
        run(delta / div_60);
        render();

        now = SDL_GetPerformanceCounter();
        delta = (double)((now - start)*1000.0) / SDL_GetPerformanceFrequency();
        start = now;
    }

    cleanup();
    return 0;

fail:
    log_error("failed: %s\n", SDL_GetError());
    cleanup();
    return -1;
}
