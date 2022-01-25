#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <SDL_render.h>


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
    TouchButtonID_OPEN,
    TouchButtonID_SAVE,
    TouchButtonID_LOAD,
    TouchButtonID_OPTIONS,
    TouchButtonID_BACK,
};

struct TouchButton
{
    const char* path;
    SDL_Texture* texture;
    int w, h;
    SDL_Rect rect;
    bool enabled;
    const bool dragable; // can slide your finger over
};

// bad name, basically it just keeps tracks of the multi touches
struct TouchCacheEntry
{
    SDL_FingerID id;
    enum TouchButtonID touch_id;
    bool down;
};

bool touch_init(SDL_Renderer* renderer);
void touch_exit(void);
void touch_render(SDL_Renderer* renderer);

void on_touch_up(SDL_FingerID id);
void on_touch_down(SDL_FingerID id, int x, int y);
void on_touch_motion(SDL_FingerID id, int x, int y);

void on_mouse_up(SDL_FingerID id);
void on_mouse_down(SDL_FingerID id, int x, int y);
void on_mouse_motion(SDL_FingerID id, int x, int y);

void on_touch_resize(int w, int h);
void on_touch_menu_change(enum MenuType new_menu);
void touch_enable(void);
void touch_disable(void);

#ifdef __cplusplus
}
#endif
