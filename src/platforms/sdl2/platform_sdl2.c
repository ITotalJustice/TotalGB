#include "ifile/ifile.h"
#include <SDL2/SDL.h>
#include <gb.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <romloader.h>
#include <filedialog.h>
#include <util.h>
#include <ifile/cfile/cfile.h>
#include <ifile/gzip/gzip.h>
#include <time.h>

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif


enum
{
    WIDTH = GB_SCREEN_WIDTH,
    HEIGHT = GB_SCREEN_HEIGHT,

    // VOLUME = SDL_MIX_MAXVOLUME / 2,
    VOLUME = 51, // 40%
    SAMPLES = 2048,
    SDL_AUDIO_FREQ = 96000,
};

enum TouchButtonID
{
    TouchButtonID_A,
    TouchButtonID_B,
    TouchButtonID_UP,
    TouchButtonID_DOWN,
    TouchButtonID_LEFT,
    TouchButtonID_RIGHT,
    TouchButtonID_START,
    TouchButtonID_SELECT,
    TouchButtonID_VOLUME,
    TouchButtonID_MENU,
    TouchButtonID_TITLE,
    TouchButtonID_SAVE,
    TouchButtonID_LOAD,
    TouchButtonID_BACK,
};

static struct TouchButton
{
    const char* path;
    SDL_Texture* texture;
    int w, h;
    SDL_Rect rect;
    bool enabled;
    const bool dragable; // can slide your finger over
} touch_buttons[] =
{
    [TouchButtonID_A] =
    {
        .path = "res/sprites/a.png",
        .w = 40,
        .h = 40,
        .dragable = true,
    },
    [TouchButtonID_B] =
    {
        .path = "res/sprites/b.png",
        .w = 40,
        .h = 40,
        .dragable = true,
    },
    [TouchButtonID_UP] =
    {
        .path = "res/sprites/up.png",
        .w = 30,
        .h = 38,
        .dragable = true,
    },
    [TouchButtonID_DOWN] =
    {
        .path = "res/sprites/down.png",
        .w = 30,
        .h = 38,
        .dragable = true,
    },
    [TouchButtonID_LEFT] =
    {
        .path = "res/sprites/left.png",
        .w = 38,
        .h = 30,
        .dragable = true,
    },
    [TouchButtonID_RIGHT] =
    {
        .path = "res/sprites/right.png",
        .w = 38,
        .h = 30,
        .dragable = true,
    },
    [TouchButtonID_START] =
    {
        .path = "res/sprites/start.png",
        .w = 108,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_SELECT] =
    {
        .path = "res/sprites/select.png",
        .w = 108,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_VOLUME] =
    {
        .path = "res/sprites/volume.png",
        .w = 48,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_MENU] =
    {
        .path = "res/sprites/menu.png",
        .w = 48,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_TITLE] =
    {
        .path = "res/sprites/title.png",
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_SAVE] =
    {
        .path = "res/sprites/save.png",
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_LOAD] =
    {
        .path = "res/sprites/load.png",
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_BACK] =
    {
        .path = "res/sprites/back.png",
        .w = 105,
        .h = 35,
        .dragable = false,
    },
};


// bad name, basically it just keeps tracks of the multi touches
struct TouchCacheEntry
{
    SDL_FingerID id;
    enum TouchButtonID touch_id;
    bool down;
};

enum RunningState
{
    RunningState_GAME,
    RunningState_MENU,
};

static struct TouchCacheEntry touch_entries[8] = {0}; // max of 8 touches at once
static struct TouchCacheEntry mouse_entries[1] = {0}; // max of 1 mouse clicks at once

static struct GB_Core gb;
static uint32_t core_pixels[HEIGHT][WIDTH];

static char rom_path[0x304] = {0};

static uint8_t rom_data[GB_ROM_SIZE_MAX] = {0};
static size_t rom_size = 0;
static bool has_rom = false;

static uint8_t sram_data[GB_SAVE_SIZE_MAX] = {0};
static size_t sram_size = 0;
static bool has_sram = false;

static bool is_mobile = false;
static bool running = true;
static enum RunningState running_state = RunningState_GAME;

static int scale = 4;
static int speed = 1;
static int frameskip_counter = 0;
static int audio_freq = 0;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Texture* prev_texture = NULL;
static SDL_AudioDeviceID audio_device = 0;
static SDL_Rect rect = {0};
static SDL_PixelFormat* pixel_format = NULL;
static SDL_GameController* game_controller = NULL;


static void toggle_fullscreen();
static void savestate(const char* path);
static void loadstate(const char* path);
static bool loadsave(const char* path, const struct GB_RomInfo* rom_info);
static bool flushsram(const char* path);
static void close_save();
static void toggle_volume();
static void core_on_apu(void* user, struct GB_ApuCallbackData* data);


static void change_running_state(enum RunningState new_state)
{
    switch (new_state)
    {
        case RunningState_GAME:
            touch_buttons[TouchButtonID_A].enabled = true;
            touch_buttons[TouchButtonID_B].enabled = true;
            touch_buttons[TouchButtonID_UP].enabled = true;
            touch_buttons[TouchButtonID_DOWN].enabled = true;
            touch_buttons[TouchButtonID_LEFT].enabled = true;
            touch_buttons[TouchButtonID_RIGHT].enabled = true;
            touch_buttons[TouchButtonID_START].enabled = true;
            touch_buttons[TouchButtonID_SELECT].enabled = true;
            touch_buttons[TouchButtonID_VOLUME].enabled = true;
            touch_buttons[TouchButtonID_MENU].enabled = true;
            touch_buttons[TouchButtonID_TITLE].enabled = false;
            touch_buttons[TouchButtonID_SAVE].enabled = false;
            touch_buttons[TouchButtonID_LOAD].enabled = false;
            touch_buttons[TouchButtonID_BACK].enabled = false;
            #ifdef EMSCRIPTEN
                SDL_PauseAudioDevice(audio_device, 0);
            #endif
            break;

        case RunningState_MENU:
            touch_buttons[TouchButtonID_A].enabled = false;
            touch_buttons[TouchButtonID_B].enabled = false;
            touch_buttons[TouchButtonID_UP].enabled = false;
            touch_buttons[TouchButtonID_DOWN].enabled = false;
            touch_buttons[TouchButtonID_LEFT].enabled = false;
            touch_buttons[TouchButtonID_RIGHT].enabled = false;
            touch_buttons[TouchButtonID_START].enabled = false;
            touch_buttons[TouchButtonID_SELECT].enabled = false;
            touch_buttons[TouchButtonID_VOLUME].enabled = false;
            touch_buttons[TouchButtonID_MENU].enabled = false;
            touch_buttons[TouchButtonID_TITLE].enabled = true;
            touch_buttons[TouchButtonID_SAVE].enabled = true;
            touch_buttons[TouchButtonID_LOAD].enabled = true;
            touch_buttons[TouchButtonID_BACK].enabled = true;
            #ifdef EMSCRIPTEN
                SDL_PauseAudioDevice(audio_device, 1);
            #endif
            break;
    }

    running_state = new_state;
}

static bool loadrom(IFile_t* file, const char* path)
{
    close_save();

    if (!file)
    {
        printf("failed to open rom\n");
        goto fail;
    }

    rom_size = ifile_size(file);

    if (!rom_size || rom_size >= sizeof(rom_data))
    {
        printf("rom is invalid size: %zu\n", rom_size);
        goto fail;
    }

    const bool result = ifile_read(file, rom_data, rom_size);
    ifile_close(file);

    if (!result)
    {
        printf("failed to read rom!\n");
        goto fail;
    }

    struct GB_RomInfo rom_info = {0};

    if (!GB_get_rom_info(rom_data, rom_size, &rom_info))
    {
        goto fail;
    }

    if (!loadsave(path, &rom_info))
    {
        goto fail;
    }

    if (!GB_loadrom(&gb, rom_data, rom_size))
    {
        printf("failed to loadrom\n");
        goto fail;
    }

    has_rom = true;
    // todo: start in menu mode once loadrom option has been added
    change_running_state(RunningState_GAME);

    memset(rom_path, 0, sizeof(rom_path));
    strncpy(rom_path, path, sizeof(rom_path) - 1);

    has_rom = true;
    return true;

fail:
    has_rom = false;
    return false;
}

static void filedialog()
{
    #ifdef EMSCRIPTEN
        EM_ASM(
            let rom_input = document.getElementById("RomFilePicker");
            rom_input.click();
        );
    #else
    const struct FileDialogResult r = filedialog_open_file("gb,gbc,zip");
    if (r.type == FileDialogResultType_OK) {
        loadrom(romloader_open(r.path), r.path);
    }
    #endif
}

#ifdef EMSCRIPTEN
static void syncfs()
{
    EM_ASM(
        FS.syncfs(function (err) {
            if (err) {
                console.log(err);
            }
        });
    );
}

EMSCRIPTEN_KEEPALIVE
void em_set_browser_type(bool _is_mobile)
{
    is_mobile = _is_mobile;
    // is_mobile = true; // for testing
}

EMSCRIPTEN_KEEPALIVE
void em_load_rom_data(const char* name, const uint8_t* data, int len)
{
    printf("[EM] loading rom! name: %s len: %d\n", name, len);

    if (len <= 0 || len > sizeof(rom_data))
    {
        return;
    }

    if (loadrom(romloader_open_mem(name, data, len), name))
    {
        EM_ASM({
            let button = document.getElementById('HackyButton');
            button.style.visibility = "hidden";
        });
        printf("[EM] loaded rom! name: %s len: %d\n", rom_path, len);
    }
}
#endif // #ifdef EMSCRIPTEN

static int get_scale(int w, int h)
{
    const int scale_w = w / WIDTH;
    const int scale_h = h / HEIGHT;

    return scale_w < scale_h ? scale_w : scale_h;
}

static void on_touch_button_change(enum TouchButtonID touch_id, bool down)
{
    switch (touch_id)
    {
        case TouchButtonID_A:        GB_set_buttons(&gb, GB_BUTTON_A, down);      break;
        case TouchButtonID_B:        GB_set_buttons(&gb, GB_BUTTON_B, down);      break;
        case TouchButtonID_UP:       GB_set_buttons(&gb, GB_BUTTON_UP, down);     break;
        case TouchButtonID_DOWN:     GB_set_buttons(&gb, GB_BUTTON_DOWN, down);   break;
        case TouchButtonID_LEFT:     GB_set_buttons(&gb, GB_BUTTON_LEFT, down);   break;
        case TouchButtonID_RIGHT:    GB_set_buttons(&gb, GB_BUTTON_RIGHT, down);  break;
        case TouchButtonID_START:    GB_set_buttons(&gb, GB_BUTTON_START, down);  break;
        case TouchButtonID_SELECT:   GB_set_buttons(&gb, GB_BUTTON_SELECT, down); break;

        case TouchButtonID_VOLUME:
            if (down)
            {
                toggle_volume();
            }
            break;

        case TouchButtonID_MENU:
            if (down)
            {
                change_running_state(RunningState_MENU);
            }
            break;

        case TouchButtonID_SAVE:
            if (down)
            {
                savestate(rom_path);
                change_running_state(RunningState_GAME);
            }
            break;

        case TouchButtonID_LOAD:
            if (down)
            {
                loadstate(rom_path);
                change_running_state(RunningState_GAME);
            }
            break;

        case TouchButtonID_BACK:
            if (down)
            {
                change_running_state(RunningState_GAME);
            }
            break;

        case TouchButtonID_TITLE:
            break;
    }
}

static int is_touch_in_range(int x, int y)
{
    for (size_t i = 0; i < ARRAY_SIZE(touch_buttons); ++i)
    {
        const struct TouchButton* e = (const struct TouchButton*)&touch_buttons[i];

        if (e->enabled)
        {
            if (x >= e->rect.x && x <= (e->rect.x + e->rect.w))
            {

                if (y >= e->rect.y && y <= (e->rect.y + e->rect.h))
                {
                    return (int)i;
                }
            }
        }
    }

    return -1;
}

static void on_touch_up(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (cache[i].down && cache[i].id == id)
        {
            cache[i].down = false;
            on_touch_button_change(cache[i].touch_id, false);

            break;
        }
    }
}

static void on_touch_down(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id, int x, int y)
{
    // check that the button press maps to a texture coord
    const int touch_id = is_touch_in_range(x, y);

    if (touch_id == -1)
    {
        return;
    }

    // find the first free entry and add it to it
    for (size_t i = 0; i < size; ++i)
    {
        if (cache[i].down == false)
        {
            cache[i].id = id;
            cache[i].touch_id = touch_id;
            cache[i].down = true;

            on_touch_button_change(cache[i].touch_id, true);

            break;
        }
    }
}

static void on_touch_motion(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id, int x, int y)
{
    // check that the button press maps to a texture coord
    const int touch_id = is_touch_in_range(x, y);

    if (touch_id == -1)
    {
        return;
    }

    // this is pretty inefficient, but its simple enough and works.
    on_touch_up(cache, size, id);

    // check if the button is dragable!
    if (touch_buttons[touch_id].dragable)
    {
        on_touch_down(cache, size, id, x, y);
    }
}

static void set_rtc_from_time_t()
{
    time_t the_time = time(NULL);
    const struct tm* tm = localtime(&the_time);

    if (!tm)
    {
        printf("failed to set rtc\n");
        return;
    }

    struct GB_Rtc rtc = {0};
    rtc.S = tm->tm_sec;
    rtc.M = tm->tm_min;
    rtc.H = tm->tm_hour;
    rtc.DL = tm->tm_yday & 0xFF;
    rtc.DH = tm->tm_yday > 0xFF;

    GB_set_rtc(&gb, rtc);
}

static void toggle_volume()
{
    #ifdef EMSCRIPTEN
        switch (SDL_GetAudioDeviceStatus(audio_device))
        {
            case SDL_AUDIO_PLAYING:
                GB_set_apu_callback(&gb, NULL, NULL, 0);
                SDL_ClearQueuedAudio(audio_device);
                SDL_PauseAudioDevice(audio_device, 1);
                printf("paused audio\n");
                break;

            case SDL_AUDIO_STOPPED:
            case SDL_AUDIO_PAUSED:
                GB_set_apu_callback(&gb, core_on_apu, NULL, audio_freq);
                SDL_PauseAudioDevice(audio_device, 0);
                printf("resumed audio audio\n");
                break;
        }
    #endif
}

static void run()
{
    if (!has_rom || running_state != RunningState_GAME)
    {
        return;
    }

    for (int i = 0; i < speed; ++i)
    {
        GB_run_frame(&gb);
    }

    static int rtc_counter = 0;

    if (rtc_counter >= 60)
    {
        set_rtc_from_time_t();
        rtc_counter = 0;
    }
    else
    {
        rtc_counter++;
    }
}

static void savestate(const char* path)
{
    printf("doing savestate\n");
    if (!has_rom)
    {
        return;
    }

    static struct GB_State state = {0};
    const struct SafeString ss = util_create_state_path(path);

    if (!ss.valid)
    {
        return;
    }
    printf("got state path %s\n", ss.str);

    IFile_t* f = igzip_open(ss.str, IFileMode_WRITE, 0);

    if (f)
    {
        GB_savestate(&gb, &state);

        ifile_write(f, &state, sizeof(state));
        ifile_close(f);

        #ifdef EMSCRIPTEN
            syncfs();
        #endif
        printf("saved state\n");
    }
    else {
        printf("failed to savestate\n");
    }
}

static void loadstate(const char* path)
{
    if (!has_rom)
    {
        return;
    }

    static struct GB_State state = {0};
    const struct SafeString ss = util_create_state_path(path);

    if (!ss.valid)
    {
        return;
    }

    IFile_t* f = igzip_open(ss.str, IFileMode_READ, 0);

    if (f)
    {
        ifile_read(f, &state, sizeof(state));
        ifile_close(f);

        GB_loadstate(&gb, &state);
    }
}

static bool is_fullscreen()
{
    const int flags = SDL_GetWindowFlags(window);

    // check if we are already in fullscreen mode
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void resize_touch_buttons(int w, int h)
{
    const float big_scale = (float)get_scale(w, h) * 0.9f;
    const float min_scale = (float)get_scale(w, h) * 0.5f;

    if (is_mobile)
    {
        touch_buttons[TouchButtonID_A].rect.x = w - 45 * big_scale;
        touch_buttons[TouchButtonID_A].rect.y = h - 45 * big_scale;
        touch_buttons[TouchButtonID_A].rect.w = touch_buttons[TouchButtonID_A].w * big_scale;
        touch_buttons[TouchButtonID_A].rect.h = touch_buttons[TouchButtonID_A].h * big_scale;

        touch_buttons[TouchButtonID_B].rect.x = w - 90 * big_scale;
        touch_buttons[TouchButtonID_B].rect.y = h - 45 * big_scale;
        touch_buttons[TouchButtonID_B].rect.w = touch_buttons[TouchButtonID_B].w * big_scale;
        touch_buttons[TouchButtonID_B].rect.h = touch_buttons[TouchButtonID_B].h * big_scale;

        touch_buttons[TouchButtonID_UP].rect.x = 30 * big_scale;
        touch_buttons[TouchButtonID_UP].rect.y = h - 82 * big_scale;
        touch_buttons[TouchButtonID_UP].rect.w = touch_buttons[TouchButtonID_UP].w * big_scale;
        touch_buttons[TouchButtonID_UP].rect.h = touch_buttons[TouchButtonID_UP].h * big_scale;

        touch_buttons[TouchButtonID_DOWN].rect.x = 30 * big_scale;
        touch_buttons[TouchButtonID_DOWN].rect.y = h - 45 * big_scale;
        touch_buttons[TouchButtonID_DOWN].rect.w = touch_buttons[TouchButtonID_DOWN].w * big_scale;
        touch_buttons[TouchButtonID_DOWN].rect.h = touch_buttons[TouchButtonID_DOWN].h * big_scale;

        touch_buttons[TouchButtonID_LEFT].rect.x = 5 * big_scale;
        touch_buttons[TouchButtonID_LEFT].rect.y = h - 60 * big_scale;
        touch_buttons[TouchButtonID_LEFT].rect.w = touch_buttons[TouchButtonID_LEFT].w * big_scale;
        touch_buttons[TouchButtonID_LEFT].rect.h = touch_buttons[TouchButtonID_LEFT].h * big_scale;

        touch_buttons[TouchButtonID_RIGHT].rect.x = 47 * big_scale;
        touch_buttons[TouchButtonID_RIGHT].rect.y = h - 60 * big_scale;
        touch_buttons[TouchButtonID_RIGHT].rect.w = touch_buttons[TouchButtonID_RIGHT].w * big_scale;
        touch_buttons[TouchButtonID_RIGHT].rect.h = touch_buttons[TouchButtonID_RIGHT].h * big_scale;

        touch_buttons[TouchButtonID_START].rect.x = (w / 2) + 5 * min_scale;
        touch_buttons[TouchButtonID_START].rect.y = 5 * min_scale;
        touch_buttons[TouchButtonID_START].rect.w = touch_buttons[TouchButtonID_START].w * min_scale;
        touch_buttons[TouchButtonID_START].rect.h = touch_buttons[TouchButtonID_START].h * min_scale;

        touch_buttons[TouchButtonID_SELECT].rect.x = (w / 2) - 113 * min_scale;
        touch_buttons[TouchButtonID_SELECT].rect.y = 5 * min_scale;
        touch_buttons[TouchButtonID_SELECT].rect.w = touch_buttons[TouchButtonID_SELECT].w * min_scale;
        touch_buttons[TouchButtonID_SELECT].rect.h = touch_buttons[TouchButtonID_SELECT].h * min_scale;

        touch_buttons[TouchButtonID_VOLUME].rect.x = w - 53 * min_scale;
        touch_buttons[TouchButtonID_VOLUME].rect.y = 5 * min_scale;
        touch_buttons[TouchButtonID_VOLUME].rect.w = touch_buttons[TouchButtonID_VOLUME].w * min_scale;
        touch_buttons[TouchButtonID_VOLUME].rect.h = touch_buttons[TouchButtonID_VOLUME].h * min_scale;

        touch_buttons[TouchButtonID_MENU].rect.x = 5 * min_scale;
        touch_buttons[TouchButtonID_MENU].rect.y = 5 * min_scale;
        touch_buttons[TouchButtonID_MENU].rect.w = touch_buttons[TouchButtonID_MENU].w * min_scale;
        touch_buttons[TouchButtonID_MENU].rect.h = touch_buttons[TouchButtonID_MENU].h * min_scale;

        touch_buttons[TouchButtonID_TITLE].rect.x = 10 * min_scale;
        touch_buttons[TouchButtonID_TITLE].rect.y = 18 * min_scale;
        touch_buttons[TouchButtonID_TITLE].rect.w = touch_buttons[TouchButtonID_TITLE].w * min_scale;
        touch_buttons[TouchButtonID_TITLE].rect.h = touch_buttons[TouchButtonID_TITLE].h * min_scale;

        touch_buttons[TouchButtonID_SAVE].rect.x = 10 * min_scale;
        touch_buttons[TouchButtonID_SAVE].rect.y = 70 * min_scale;
        touch_buttons[TouchButtonID_SAVE].rect.w = touch_buttons[TouchButtonID_SAVE].w * min_scale;
        touch_buttons[TouchButtonID_SAVE].rect.h = touch_buttons[TouchButtonID_SAVE].h * min_scale;

        touch_buttons[TouchButtonID_LOAD].rect.x = 10 * min_scale;
        touch_buttons[TouchButtonID_LOAD].rect.y = (70 * min_scale) + ((45 * 1) * min_scale);
        touch_buttons[TouchButtonID_LOAD].rect.w = touch_buttons[TouchButtonID_LOAD].w * min_scale;
        touch_buttons[TouchButtonID_LOAD].rect.h = touch_buttons[TouchButtonID_LOAD].h * min_scale;

        touch_buttons[TouchButtonID_BACK].rect.x = 10 * min_scale;
        touch_buttons[TouchButtonID_BACK].rect.y = (70 * min_scale) + ((45 * 2) * min_scale);
        touch_buttons[TouchButtonID_BACK].rect.w = touch_buttons[TouchButtonID_BACK].w * min_scale;
        touch_buttons[TouchButtonID_BACK].rect.h = touch_buttons[TouchButtonID_BACK].h * min_scale;
    }
    else
    {
        for (size_t i = 0; i < ARRAY_SIZE(touch_buttons); ++i)
        {
            touch_buttons[i].rect.x = -8000;
            touch_buttons[i].rect.y = -8000;
            touch_buttons[i].enabled = false;
        }
    }
}

static void setup_rect(int w, int h)
{
    const int min_scale = get_scale(w, h);

    rect.w = WIDTH * min_scale;
    rect.h = HEIGHT * min_scale;
    rect.x = (w - rect.w);
    rect.y = (h - rect.h);

    // don't divide by zero!
    if (rect.x > 0) rect.x /= 2;
    if (rect.y > 0) rect.y /= 2;
}

static void scale_screen()
{
    SDL_SetWindowSize(window, WIDTH * scale, HEIGHT * scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void toggle_fullscreen()
{
    // check if we are already in fullscreen mode
    if (is_fullscreen())
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }
}

static void on_ctrl_key_event(const SDL_KeyboardEvent* e, bool down)
{
    if (down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_EQUALS:
            case SDL_SCANCODE_KP_PLUS:
                ++scale;
                scale_screen();
                break;

            case SDL_SCANCODE_MINUS:
            case SDL_SCANCODE_KP_PLUSMINUS:
            case SDL_SCANCODE_KP_MINUS:
                scale = scale > 0 ? scale - 1 : 1;
                scale_screen();
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
                speed = (e->keysym.scancode - SDL_SCANCODE_1) + 1;
                break;

            case SDL_SCANCODE_F:
                toggle_fullscreen();
                break;

            case SDL_SCANCODE_L:
                loadstate(rom_path);
                break;

            case SDL_SCANCODE_S:
                savestate(rom_path);
                break;

            case SDL_SCANCODE_O:
                filedialog();
                break;

            default: break; // silence enum warning
        }
    }
}

static void on_key_event(const SDL_KeyboardEvent* e)
{
    const bool down = e->type == SDL_KEYDOWN;
    const bool ctrl = (e->keysym.mod & KMOD_CTRL) > 0;

    if (ctrl)
    {
        on_ctrl_key_event(e, down);

        return;
    }

    if (!has_rom)
    {
        return;
    }

    switch (e->keysym.scancode)
    {
        case SDL_SCANCODE_X:        GB_set_buttons(&gb, GB_BUTTON_A, down);        break;
        case SDL_SCANCODE_Z:        GB_set_buttons(&gb, GB_BUTTON_B, down);        break;
        case SDL_SCANCODE_RETURN:   GB_set_buttons(&gb, GB_BUTTON_START, down);    break;
        case SDL_SCANCODE_SPACE:    GB_set_buttons(&gb, GB_BUTTON_SELECT, down);   break;
        case SDL_SCANCODE_UP:       GB_set_buttons(&gb, GB_BUTTON_UP, down);       break;
        case SDL_SCANCODE_DOWN:     GB_set_buttons(&gb, GB_BUTTON_DOWN, down);     break;
        case SDL_SCANCODE_LEFT:     GB_set_buttons(&gb, GB_BUTTON_LEFT, down);     break;
        case SDL_SCANCODE_RIGHT:    GB_set_buttons(&gb, GB_BUTTON_RIGHT, down);    break;

    #ifndef EMSCRIPTEN
        case SDL_SCANCODE_ESCAPE:
            running = false;
            break;
    #endif // EMSCRIPTEN

        default: break; // silence enum warning
    }
}

static void on_controller_axis_event(const SDL_ControllerAxisEvent* e)
{
    enum
    {
        deadzone = 8000,
        left     = -deadzone,
        right    = +deadzone,
        up       = -deadzone,
        down     = +deadzone,
    };

    switch (e->axis)
    {
        case SDL_CONTROLLER_AXIS_LEFTX: case SDL_CONTROLLER_AXIS_RIGHTX:
            if (e->value < left)
            {
                GB_set_buttons(&gb, GB_BUTTON_LEFT, true);
                GB_set_buttons(&gb, GB_BUTTON_RIGHT, false);
            }
            else if (e->value > right)
            {
                GB_set_buttons(&gb, GB_BUTTON_LEFT, false);
                GB_set_buttons(&gb, GB_BUTTON_RIGHT, true);
            }
            else
            {
                GB_set_buttons(&gb, GB_BUTTON_LEFT, false);
                GB_set_buttons(&gb, GB_BUTTON_RIGHT, false);
            }
            break;

        case SDL_CONTROLLER_AXIS_LEFTY: case SDL_CONTROLLER_AXIS_RIGHTY:
            if (e->value < up)
            {
                GB_set_buttons(&gb, GB_BUTTON_UP, true);
                GB_set_buttons(&gb, GB_BUTTON_DOWN, false);
            }
            else if (e->value > down)
            {
                GB_set_buttons(&gb, GB_BUTTON_UP, false);
                GB_set_buttons(&gb, GB_BUTTON_DOWN, true);
            }
            else
            {
                GB_set_buttons(&gb, GB_BUTTON_UP, false);
                GB_set_buttons(&gb, GB_BUTTON_DOWN, false);
            }
            break;
    }
}

static void on_controller_event(const SDL_ControllerButtonEvent* e)
{
    const bool down = e->type == SDL_CONTROLLERBUTTONDOWN;

    switch (e->button)
    {
        case SDL_CONTROLLER_BUTTON_A:               GB_set_buttons(&gb, GB_BUTTON_A, down);         break;
        case SDL_CONTROLLER_BUTTON_B:               GB_set_buttons(&gb, GB_BUTTON_B, down);         break;
        case SDL_CONTROLLER_BUTTON_X:               break;
        case SDL_CONTROLLER_BUTTON_Y:               break;
        case SDL_CONTROLLER_BUTTON_START:           GB_set_buttons(&gb, GB_BUTTON_START, down);     break;
        case SDL_CONTROLLER_BUTTON_BACK:            GB_set_buttons(&gb, GB_BUTTON_SELECT, down);    break;
        case SDL_CONTROLLER_BUTTON_GUIDE:           break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:    break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:       break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:   break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:      break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:         GB_set_buttons(&gb, GB_BUTTON_UP, down);        break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:       GB_set_buttons(&gb, GB_BUTTON_DOWN, down);      break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:       GB_set_buttons(&gb, GB_BUTTON_LEFT, down);      break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:      GB_set_buttons(&gb, GB_BUTTON_RIGHT, down);     break;
    }
}

static void on_controller_device_event(const SDL_ControllerDeviceEvent* e)
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
            break;
    }
}

static void on_touch_event(const SDL_TouchFingerEvent* e)
{
    int win_w = 0, win_h = 0;

    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    // we need to un-normalise x, y
    const int x = e->x * win_w;
    const int y = e->y * win_h;

    switch (e->type)
    {
        case SDL_FINGERUP:
            on_touch_up(touch_entries, ARRAY_SIZE(touch_entries), e->fingerId);
            break;

        case SDL_FINGERDOWN:
            on_touch_down(touch_entries, ARRAY_SIZE(touch_entries), e->fingerId, x, y);
            break;

        case SDL_FINGERMOTION:
            on_touch_motion(touch_entries, ARRAY_SIZE(touch_entries), e->fingerId, x, y);
            break;
    }
}

static void on_mouse_button_event(const SDL_MouseButtonEvent* e)
{
    // we already handle touch events...
    if (e->which == SDL_TOUCH_MOUSEID)
    {
        return;
    }

    switch (e->type)
    {
        case SDL_MOUSEBUTTONUP:
            on_touch_up(mouse_entries, ARRAY_SIZE(mouse_entries), e->which);
            break;

        case SDL_MOUSEBUTTONDOWN:
            on_touch_down(mouse_entries, ARRAY_SIZE(mouse_entries), e->which, e->x, e->y);
            break;
    }
}

static void on_mouse_motion_event(const SDL_MouseMotionEvent* e)
{
    // we already handle touch events!
    if (e->which == SDL_TOUCH_MOUSEID)
    {
        return;
    }

    // only handle left clicks!
    if (e->state & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        on_touch_motion(mouse_entries, ARRAY_SIZE(mouse_entries), e->which, e->x, e->y);
    }
}

static void on_window_event(const SDL_WindowEvent* e)
{
    switch (e->event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            // use this rather than window size because iirc i had issues with
            // hi-dpi screens.
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);

            setup_rect(w, h);
            resize_touch_buttons(w, h);
        }   break;
    }
}

static void events()
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                running = false;
                return;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                on_key_event(&e.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                on_controller_event(&e.cbutton);
                break;

            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
                on_controller_device_event(&e.cdevice);
                break;

            case SDL_CONTROLLERAXISMOTION:
                on_controller_axis_event(&e.caxis);
                break;

            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                on_mouse_button_event(&e.button);
                break;

             case SDL_MOUSEMOTION:
                on_mouse_motion_event(&e.motion);
                break;

            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                on_touch_event(&e.tfinger);
                break;

            case SDL_WINDOWEVENT:
                on_window_event(&e.window);
                break;
        }
    }
}

static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    (void)user;

    // using buffers because pushing 1 sample at a time seems to
    // cause popping sounds (on my chromebook).
    static int8_t buffer[SAMPLES * 2] = {0};
    static size_t buffer_count = 0;

    static bool first = true;

    if (first)
    {
        // stops nasty audio pop in the beginning on chromebook
        SDL_QueueAudio(audio_device, buffer, sizeof(buffer));
        SDL_QueueAudio(audio_device, buffer, sizeof(buffer));
        SDL_QueueAudio(audio_device, buffer, sizeof(buffer));
        SDL_QueueAudio(audio_device, buffer, sizeof(buffer));
        first = false;
    }

    // if speedup is enabled, skip x many samples in order to not fill the audio buffer!
    if (speed > 1)
    {
        static int skipped_samples = 0;

        if (skipped_samples < speed - 1)
        {
            ++skipped_samples;
            return;
        }

        skipped_samples = 0;
    }

    #ifdef EMSCRIPTEN
        if (SDL_GetQueuedAudioSize(audio_device) > sizeof(buffer) * 4)
        {
            return;
        }
    #endif

    const int16_t left = (data->ch1[0] + data->ch2[0] + data->ch3[0] + data->ch4[0]) * data->left_amp;
    const int16_t right = (data->ch1[1] + data->ch2[1] + data->ch3[1] + data->ch4[1]) * data->right_amp;

    buffer[buffer_count++] = left / 4;
    buffer[buffer_count++] = right / 4;

    if (buffer_count == sizeof(buffer))
    {
        buffer_count = 0;

        uint8_t samples[sizeof(buffer)] = {0};

        SDL_MixAudioFormat(samples, (const uint8_t*)buffer, AUDIO_S8, sizeof(buffer), VOLUME);

        #ifndef EMSCRIPTEN
            while (SDL_GetQueuedAudioSize(audio_device) > sizeof(buffer))
            {
                SDL_Delay(4);
            }
        #endif

        SDL_QueueAudio(audio_device, samples, sizeof(samples));
    }
}

static uint32_t core_on_colour(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b)
{
    (void)user;

    uint32_t R = 0, G = 0, B = 0;

    switch (type)
    {
        case GB_ColourCallbackType_DMG:
            R = r;
            G = g;
            B = b;
            break;

        case GB_ColourCallbackType_GBC:
            #if 1
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

static void core_on_vblank(void* user)
{
    (void)user;

    ++frameskip_counter;

    if (frameskip_counter >= speed)
    {
        SDL_Texture* tmp = prev_texture;
        prev_texture = texture;
        texture = tmp;

        void* pixels; int pitch;

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        SDL_SetTextureBlendMode(prev_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(prev_texture, 100);

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, core_pixels, sizeof(core_pixels));
        SDL_UnlockTexture(texture);

        frameskip_counter = 0;
    }
}

static void load_touch_buttons()
{
    #ifndef EMSCRIPTEN
        return;
    #endif

    for (size_t i = 0; i < ARRAY_SIZE(touch_buttons); ++i)
    {
        SDL_Surface* surface = IMG_Load(touch_buttons[i].path);

        if (surface)
        {
            touch_buttons[i].texture = SDL_CreateTextureFromSurface(renderer, surface);
            touch_buttons[i].rect.w = touch_buttons[i].w;
            touch_buttons[i].rect.h = touch_buttons[i].h;

            // todo: reduce the alpha in the src pictures
            if (i <= TouchButtonID_MENU)
            {
                SDL_SetTextureBlendMode(touch_buttons[i].texture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(touch_buttons[i].texture, 80);
            }

            SDL_FreeSurface(surface);
        }
        else
        {
            printf("failed to load: %s\n", SDL_GetError());
        }
    }
}

static void render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // todo:
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_RenderCopy(renderer, prev_texture, NULL, &rect);

    if (running_state == RunningState_MENU)
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);

        const float min_scale = (float)get_scale(w, h) * 0.5f;

        SDL_Rect r = { .x = 0, .y = 10 * min_scale, .w = 130 * min_scale, .h = h - 20 * min_scale };
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &r);
    }

    for (size_t i = 0; i < ARRAY_SIZE(touch_buttons); ++i)
    {
        if (touch_buttons[i].texture && touch_buttons[i].enabled)
        {
            SDL_RenderCopy(renderer, touch_buttons[i].texture, NULL, &touch_buttons[i].rect);
        }
    }

    SDL_RenderPresent(renderer);
}

#ifdef EMSCRIPTEN
static void em_loop()
{
    if (has_rom && GB_has_save(&gb))
    {
        static int sram_sync_counter = 0;

        // sync saves every 1.5s
        if (sram_sync_counter >= 90)
        {
            flushsram(rom_path);
            sram_sync_counter = 0;
        }
        else
        {
            sram_sync_counter++;
        }
    }

    events();
    run();
    render();
}
#endif // #ifdef EMSCRIPTEN

static bool flushsram(const char* path)
{
    if (has_sram)
    {
        const struct SafeString ss = util_create_save_path(path);

        IFile_t* file = icfile_open(ss.str, IFileMode_WRITE, 0);

        if (file)
        {
            const bool result = ifile_write(file, sram_data, sram_size);
            ifile_close(file);

            if (result)
            {
                printf("flushed save: %s\n", ss.str);
            }
            else
            {
                printf("failed to write save: %s %s\n", ss.str, SDL_GetError());
                return false;
            }
        }
        else
        {
            printf("no open savefile for writing: %s %s\n", ss.str, SDL_GetError());
            return false;
        }

        #ifdef EMSCRIPTEN
            syncfs();
        #endif
    }

    return true;
}

static void close_save()
{
    flushsram(rom_path);
}

static bool loadsave(const char* path, const struct GB_RomInfo* rom_info)
{
    close_save();

    has_sram = false;

    if (!rom_info->ram_size)
    {
        return true;
    }

    if (rom_info->flags & MBC_FLAGS_BATTERY)
    {
        const struct SafeString ss = util_create_save_path(path);

        IFile_t* file = icfile_open(ss.str, IFileMode_READ, 0);

        if (file)
        {
            if (ifile_read(file, sram_data, rom_info->ram_size))
            {
                printf("loaded save %s\n", ss.str);
            }
            else
            {
                printf("failed to load save %s %s\n", ss.str, SDL_GetError());
            }

            ifile_close(file);
        }
        else
        {
            printf("no save found %s %s\n", ss.str, SDL_GetError());
        }

        has_sram = true;
    }

    sram_size = rom_info->ram_size;

    GB_set_sram(&gb, sram_data, rom_info->ram_size);

    return true;
}

static void cleanup()
{
    close_save();

    if (pixel_format)   { SDL_free(pixel_format); }
    if (audio_device)   { SDL_CloseAudioDevice(audio_device); }
    if (prev_texture)   { SDL_DestroyTexture(prev_texture); }
    if (texture)        { SDL_DestroyTexture(texture); }
    if (renderer)       { SDL_DestroyRenderer(renderer); }
    if (window)         { SDL_DestroyWindow(window); }

    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char** argv)
{
    // SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("failed to init sdl\n");
        goto fail;
    }

    for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        const char* driver_name = SDL_GetAudioDriver(i);
        printf("Audio driver aval: %s\n", SDL_GetAudioDriver(i));
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        printf("IMG_Init: %s\n", IMG_GetError());
        goto fail;
    }

    if (SDL_GameControllerAddMappingsFromFile("res/controller_mapping/gamecontrollerdb.txt"))
    {
        printf("failed to open controllerdb file! %s\n", SDL_GetError());
    }

#ifdef EMSCRIPTEN
    SDL_DisplayMode display = { 0 };
    SDL_GetCurrentDisplayMode(0, &display);
    window = SDL_CreateWindow("TotalGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display.w, display.h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
#else
    window = SDL_CreateWindow("TotalGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
#endif

    if (!window)
    {
        printf("failed to create window\n");
        goto fail;
    }

    const uint32_t pixel_format_enum = SDL_GetWindowPixelFormat(window);

    pixel_format = SDL_AllocFormat(pixel_format_enum);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    prev_texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);

    if (!texture || !prev_texture)
    {
        printf("failed to create texture\n");
        goto fail;
    }

    load_touch_buttons();

    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);

        setup_rect(w, h);
        resize_touch_buttons(w, h);
    }

    const SDL_AudioSpec wanted =
    {
        .freq = SDL_AUDIO_FREQ,
        .format = AUDIO_S8,
        .channels = 2,
        .silence = 0,
        .samples = SAMPLES,
        .padding = 0,
        .size = 0,
        .callback = NULL,
        .userdata = NULL,
    };

    SDL_AudioSpec aspec_got = { 0 };

    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &aspec_got, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    if (audio_device == 0)
    {
        printf("failed to open audio device\n");
        goto fail;
    }

    printf("[SDL-AUDIO] freq: %d\n", aspec_got.freq);
    printf("[SDL-AUDIO] channels: %d\n", aspec_got.channels);
    printf("[SDL-AUDIO] samples: %d\n", aspec_got.samples);
    printf("[SDL-AUDIO] size: %d\n", aspec_got.size);

    SDL_PauseAudioDevice(audio_device, 0);

    if (!GB_init(&gb))
    {
        goto fail;
    }

    #ifdef EMSCRIPTEN
        audio_freq = aspec_got.freq + 512;
    #else
        audio_freq = aspec_got.freq;
    #endif

    GB_set_apu_callback(&gb, core_on_apu, NULL, audio_freq);
    GB_set_vblank_callback(&gb, core_on_vblank, NULL);
    GB_set_colour_callback(&gb, core_on_colour, NULL);
    GB_set_pixels(&gb, core_pixels, GB_SCREEN_WIDTH, 32);
    GB_set_rtc_update_config(&gb, GB_RTC_UPDATE_CONFIG_NONE);

    #ifdef EMSCRIPTEN
        EM_ASM(
            FS.mkdir("/saves"); FS.mount(IDBFS, {}, "/saves");
            FS.mkdir("/states"); FS.mount(IDBFS, {}, "/states");

            FS.syncfs(true, function (err) {
                if (err) {
                    console.log(err);
                }
            });

            if (IsMobileBrowser()) {
                console.log("is a mobile browser");
                _em_set_browser_type(true);
            } else {
                console.log("is NOT a mobile browser");
                _em_set_browser_type(false);
            }
        );

        // todo: start in menu mode once loadrom option has been added
        change_running_state(RunningState_GAME);
        emscripten_set_main_loop(em_loop, 0, true);
    #else
        if (argc < 2)
        {
            filedialog();
            if (!has_rom)
            {
                printf("missing args\n");
                goto fail;
            }
        }
        else if (!loadrom(romloader_open(argv[1]), argv[1]))
        {
            goto fail;
        }

        while (running)
        {
            events();
            run();
            render();
        }
    #endif

    cleanup();

    return 0;

fail:
    printf("fail %s\n", SDL_GetError());
    cleanup();

    return -1;
}
