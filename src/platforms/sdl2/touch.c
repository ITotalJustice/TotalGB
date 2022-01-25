#include "mgb.h"
#include "main.h"
#include "touch.h"
#include <stdint.h>
#include <util.h>


#if defined(__ANDROID__)
#define ASSETS_FOLDER(str) ("sprites/" str)
#elif defined(__SWITCH__)
#define ASSETS_FOLDER(str) ("romfs:/" str)
#else
#define ASSETS_FOLDER(str) ("res/sprites/" str)
#endif

static struct TouchButton touch_buttons[] =
{
    [TouchButtonID_A] =
    {
        .path = ASSETS_FOLDER("a.bmp"),
        .w = 40,
        .h = 40,
        .dragable = true,
    },
    [TouchButtonID_B] =
    {
        .path = ASSETS_FOLDER("b.bmp"),
        .w = 40,
        .h = 40,
        .dragable = true,
    },
    [TouchButtonID_UP] =
    {
        .path = ASSETS_FOLDER("up.bmp"),
        .w = 30,
        .h = 38,
        .dragable = true,
    },
    [TouchButtonID_DOWN] =
    {
        .path = ASSETS_FOLDER("down.bmp"),
        .w = 30,
        .h = 38,
        .dragable = true,
    },
    [TouchButtonID_LEFT] =
    {
        .path = ASSETS_FOLDER("left.bmp"),
        .w = 38,
        .h = 30,
        .dragable = true,
    },
    [TouchButtonID_RIGHT] =
    {
        .path = ASSETS_FOLDER("right.bmp"),
        .w = 38,
        .h = 30,
        .dragable = true,
    },
    [TouchButtonID_START] =
    {
        .path = ASSETS_FOLDER("start.bmp"),
        .w = 108,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_SELECT] =
    {
        .path = ASSETS_FOLDER("select.bmp"),
        .w = 108,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_VOLUME] =
    {
        .path = ASSETS_FOLDER("volume.bmp"),
        .w = 48,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_MENU] =
    {
        .path = ASSETS_FOLDER("menu.bmp"),
        .w = 48,
        .h = 48,
        .dragable = false,
    },
    [TouchButtonID_TITLE] =
    {
        .path = ASSETS_FOLDER("title.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_OPEN] =
    {
        .path = ASSETS_FOLDER("open.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_SAVE] =
    {
        .path = ASSETS_FOLDER("save.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_LOAD] =
    {
        .path = ASSETS_FOLDER("load.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_OPTIONS] =
    {
        .path = ASSETS_FOLDER("options.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
    [TouchButtonID_BACK] =
    {
        .path = ASSETS_FOLDER("back.bmp"),
        .w = 105,
        .h = 35,
        .dragable = false,
    },
};

static struct TouchCacheEntry touch_entries[8] = {0}; // max of 8 touches at once
static struct TouchCacheEntry mouse_entries[1] = {0}; // max of 1 mouse clicks at once
static SDL_Haptic* haptic = NULL;
static bool buttons_disabled = false;
static bool has_rumble = false;


static void on_touch_button_change(enum TouchButtonID touch_id, bool down)
{
    if (down && has_rumble)
    {
        SDL_HapticRumblePlay(haptic, 0.1, 25);
    }

    switch (touch_id)
    {
        case TouchButtonID_A:        set_emu_button(GB_BUTTON_A, down);      break;
        case TouchButtonID_B:        set_emu_button(GB_BUTTON_B, down);      break;
        case TouchButtonID_UP:       set_emu_button(GB_BUTTON_UP, down);     break;
        case TouchButtonID_DOWN:     set_emu_button(GB_BUTTON_DOWN, down);   break;
        case TouchButtonID_LEFT:     set_emu_button(GB_BUTTON_LEFT, down);   break;
        case TouchButtonID_RIGHT:    set_emu_button(GB_BUTTON_RIGHT, down);  break;
        case TouchButtonID_START:    set_emu_button(GB_BUTTON_START, down);  break;
        case TouchButtonID_SELECT:   set_emu_button(GB_BUTTON_SELECT, down); break;

        case TouchButtonID_VOLUME:
            if (!down)
            {
                toggle_vsync();
                // toggle_volume();
            }
            break;

        case TouchButtonID_MENU:
            if (!down)
            {
                change_menu(MenuType_SIDEBAR);
            }
            break;

        case TouchButtonID_OPEN:
            if (!down)
            {
                #ifdef __ANDROID__
                android_file_picker();
                #else
                mgb_load_rom_filedialog();
                #endif
                change_menu(MenuType_ROM);
            }
            break;

        case TouchButtonID_SAVE:
            if (!down)
            {
                savestate(NULL);
                change_menu(MenuType_ROM);
            }
            break;

        case TouchButtonID_LOAD:
            if (!down)
            {
                loadstate(NULL);
                change_menu(MenuType_ROM);
            }
            break;

        case TouchButtonID_OPTIONS:
            if (!down)
            {
                #ifdef ANDROID
                android_settings_menu();
                #endif
            }
            break;

        case TouchButtonID_BACK:
            if (!down)
            {
                change_menu(MenuType_ROM);
            }
            break;

        case TouchButtonID_TITLE:
            if (!down)
            {
                on_title_click();
            }
            break;
    }
}

static int is_touch_in_range(int x, int y)
{
    for (size_t i = 0; i < SDL_arraysize(touch_buttons); ++i)
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

static void on_touch_up_internal(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id)
{
    log_info("[TOUCH-UP] x: %d y: %d\n", 69, 69);

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

static void on_touch_down_internal(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id, int x, int y)
{
    log_info("[TOUCH-DOWN] x: %d y: %d\n", x, y);

    if (buttons_disabled)
    {
        // re-enable buttons on touch!
        touch_enable();
    }

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

static void on_touch_motion_internal(struct TouchCacheEntry* cache, size_t size, SDL_FingerID id, int x, int y)
{
    log_info("[TOUCH-MOTION] x: %d y: %d\n", x, y);

    // check that the button press maps to a texture coord
    const int touch_id = is_touch_in_range(x, y);

    if (touch_id == -1)
    {
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        if (cache[i].touch_id == (enum TouchButtonID)touch_id)
        {
            return;
        }
    }

    // this is pretty inefficient, but its simple enough and works.
    on_touch_up_internal(cache, size, id);

    // check if the button is dragable!
    if (touch_buttons[touch_id].dragable)
    {
        on_touch_down_internal(cache, size, id, x, y);
    }
}

void on_touch_up(SDL_FingerID id)
{
    on_touch_up_internal(touch_entries, SDL_arraysize(touch_entries), id);
}

void on_touch_down(SDL_FingerID id, int x, int y)
{
    on_touch_down_internal(touch_entries, SDL_arraysize(touch_entries), id, x, y);
}

void on_touch_motion(SDL_FingerID id, int x, int y)
{
    on_touch_motion_internal(touch_entries, SDL_arraysize(touch_entries), id, x, y);
}

void on_mouse_up(SDL_FingerID id)
{
    on_touch_up_internal(mouse_entries, SDL_arraysize(mouse_entries), id);
}
void on_mouse_down(SDL_FingerID id, int x, int y)
{
    on_touch_down_internal(mouse_entries, SDL_arraysize(mouse_entries), id, x, y);
}
void on_mouse_motion(SDL_FingerID id, int x, int y)
{
    on_touch_motion_internal(mouse_entries, SDL_arraysize(mouse_entries), id, x, y);
}

bool touch_init(SDL_Renderer* renderer)
{
    char* base_path = NULL;
#if !defined(ANDROID) || !defined(__EMSCRIPTEN__)
    base_path = SDL_GetBasePath();
#endif

    for (size_t i = 0; i < SDL_arraysize(touch_buttons); ++i)
    {
        struct SafeString ss = {0};

        if (base_path)
        {
            ss = ss_build("%s/%s", base_path, touch_buttons[i].path);
        }
        else
        {
            ss = ss_build(touch_buttons[i].path);
        }

        SDL_Surface* surface = SDL_LoadBMP(ss.str);

        if (surface)
        {
            touch_buttons[i].texture = SDL_CreateTextureFromSurface(renderer, surface);
            touch_buttons[i].rect.w = touch_buttons[i].w;
            touch_buttons[i].rect.h = touch_buttons[i].h;

            // todo: reduce the alpha in the src pictures
            if (i <= TouchButtonID_MENU)
            {
                // for testing
                touch_buttons[i].enabled = true;
                SDL_SetTextureBlendMode(touch_buttons[i].texture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(touch_buttons[i].texture, 80);
            }

            SDL_FreeSurface(surface);
        }
        else
        {
            log_error("[TOUCH] failed to load image: %s reason: %s\n", ss.str, SDL_GetError());
            goto fail;
        }
    }

    for (int i = 0; i < SDL_NumHaptics(); i++)
    {
        log_info("[SDL-HAPTICS] index: %d name: %s\n", i, SDL_HapticName(i));

        haptic = SDL_HapticOpen(i);
        if (!haptic)
        {
            log_error("[SDL-HAPTICS] failed to open: %s\n", SDL_GetError());
        }
        else
        {
            if (SDL_HapticRumbleSupported(haptic) == SDL_FALSE)
            {
                log_error("[SDL-HAPTICS] rumble not supported: %s\n", SDL_GetError());
            }
            else
            {
                if (SDL_HapticRumbleInit(haptic))
                {
                    log_error("[SDL-HAPTICS] failed to init rumble: %s\n", SDL_GetError());
                }
                else
                {
                    log_info("[SDL-HAPTICS] inited rumble!\n");
                    has_rumble = true;
                    break;
                }
            }
        }
    }

    if (base_path)
    {
        SDL_free(base_path);
    }

    return true;

fail:
    if (base_path)
    {
        SDL_free(base_path);
    }
    return false;
}

void touch_exit(void)
{
    has_rumble = false;

    if (haptic)
    {
        SDL_HapticClose(haptic);
        haptic = NULL;
    }

    for (size_t i = 0; i < SDL_arraysize(touch_buttons); ++i)
    {
        if (touch_buttons[i].texture)
        {
            SDL_DestroyTexture(touch_buttons[i].texture);
            touch_buttons[i].texture = NULL;
        }
    }
}

static float get_scale2(float minw, float minh, float w, float h)
{
    const float scale_w = w / minw;
    const float scale_h = h / minh;

    return SDL_min(scale_w, scale_h);
}

void touch_render(SDL_Renderer* renderer)
{
    if (get_menu_type() == MenuType_SIDEBAR)
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        const float min_scale = get_scale2(105, 35 * 8, w, h);

        SDL_Rect r = { .x = 0, .y = 10 * min_scale, .w = 130 * min_scale, .h = h - 13 * min_scale };
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &r);
    }

    for (size_t i = 0; i < SDL_arraysize(touch_buttons); ++i)
    {
        if (touch_buttons[i].texture && touch_buttons[i].enabled)
        {
            SDL_RenderCopy(renderer, touch_buttons[i].texture, NULL, &touch_buttons[i].rect);
        }
    }
}

static void set_buttons_enable(bool enable)
{
    for (size_t i = 0; i <= TouchButtonID_MENU; i++)
    {
        touch_buttons[i].enabled = enable;
    }
}

static void set_menu_buttons_enable(bool enable)
{
    for (size_t i = TouchButtonID_MENU+1; i < SDL_arraysize(touch_buttons); i++)
    {
        touch_buttons[i].enabled = enable;
    }
}


void on_touch_menu_change(enum MenuType new_menu)
{
    switch (new_menu)
    {
        case MenuType_ROM:
            set_buttons_enable(true);
            set_menu_buttons_enable(false);
            break;

        case MenuType_SIDEBAR:
            set_buttons_enable(false);
            set_menu_buttons_enable(true);
            break;
    }
}

void touch_enable(void)
{
    on_touch_menu_change(get_menu_type());
    buttons_disabled = false;
}

void touch_disable(void)
{
    // disable buttons as user is likely using keyboard or controller!
    set_buttons_enable(false);
    set_menu_buttons_enable(false);
    buttons_disabled = true;
}

void on_touch_resize(int w, int h)
{
    const float big_scale = get_scale(w, h) * 0.9f;
    const float min_scale = get_scale(w, h) * 0.5f;

    const float t = get_scale2(105, 35 * 8, w, h);

    if (1)
    {
        // 5am code
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

        touch_buttons[TouchButtonID_START].rect.x = ((float)w / 2) + 5 * min_scale;
        touch_buttons[TouchButtonID_START].rect.y = 5 * min_scale;
        touch_buttons[TouchButtonID_START].rect.w = touch_buttons[TouchButtonID_START].w * min_scale;
        touch_buttons[TouchButtonID_START].rect.h = touch_buttons[TouchButtonID_START].h * min_scale;

        touch_buttons[TouchButtonID_SELECT].rect.x = ((float)w / 2) - 113 * min_scale;
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

        touch_buttons[TouchButtonID_TITLE].rect.x = 10 * t;
        touch_buttons[TouchButtonID_TITLE].rect.y = 18 * t;
        touch_buttons[TouchButtonID_TITLE].rect.w = touch_buttons[TouchButtonID_TITLE].w * t;
        touch_buttons[TouchButtonID_TITLE].rect.h = touch_buttons[TouchButtonID_TITLE].h * t;

        touch_buttons[TouchButtonID_OPEN].rect.x = 10 * t;
        touch_buttons[TouchButtonID_OPEN].rect.y = (63 * t) + ((43 * 0) * t);
        touch_buttons[TouchButtonID_OPEN].rect.w = touch_buttons[TouchButtonID_OPEN].w * t;
        touch_buttons[TouchButtonID_OPEN].rect.h = touch_buttons[TouchButtonID_OPEN].h * t;

        touch_buttons[TouchButtonID_SAVE].rect.x = 10 * t;
        touch_buttons[TouchButtonID_SAVE].rect.y = (63 * t) + ((43 * 1) * t);
        touch_buttons[TouchButtonID_SAVE].rect.w = touch_buttons[TouchButtonID_SAVE].w * t;
        touch_buttons[TouchButtonID_SAVE].rect.h = touch_buttons[TouchButtonID_SAVE].h * t;

        touch_buttons[TouchButtonID_LOAD].rect.x = 10 * t;
        touch_buttons[TouchButtonID_LOAD].rect.y = (63 * t) + ((43 * 2) * t);
        touch_buttons[TouchButtonID_LOAD].rect.w = touch_buttons[TouchButtonID_LOAD].w * t;
        touch_buttons[TouchButtonID_LOAD].rect.h = touch_buttons[TouchButtonID_LOAD].h * t;

        touch_buttons[TouchButtonID_OPTIONS].rect.x = 10 * t;
        touch_buttons[TouchButtonID_OPTIONS].rect.y = (63 * t) + ((43 * 3) * t);
        touch_buttons[TouchButtonID_OPTIONS].rect.w = touch_buttons[TouchButtonID_OPTIONS].w * t;
        touch_buttons[TouchButtonID_OPTIONS].rect.h = touch_buttons[TouchButtonID_OPTIONS].h * t;

        touch_buttons[TouchButtonID_BACK].rect.x = 10 * t;
        touch_buttons[TouchButtonID_BACK].rect.y = (63 * t) + ((43 * 4) * t);
        touch_buttons[TouchButtonID_BACK].rect.w = touch_buttons[TouchButtonID_BACK].w * t;
        touch_buttons[TouchButtonID_BACK].rect.h = touch_buttons[TouchButtonID_BACK].h * t;
    }
    else
    {
        // basically disable them
        for (size_t i = 0; i < SDL_arraysize(touch_buttons); ++i)
        {
            touch_buttons[i].enabled = false;
        }

        buttons_disabled = true;
    }
}
