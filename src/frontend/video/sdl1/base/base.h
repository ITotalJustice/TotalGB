#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../interface.h"
#include <SDL/SDL.h>


struct BaseEventCallbacks {
	void* user;
	void (*on_resize)(void* user, int w, int h);
};

struct BaseConfig {
	struct VideoInterfaceUserCallbacks user_callbacks;
	struct BaseEventCallbacks event_callbacks;
	const char* const window_name;
	int window_flags;
	int w, h;
	int bpp;
};

struct Base {
	struct BaseEventCallbacks event_callbacks;
    struct VideoInterfaceUserCallbacks user_callbacks;
    SDL_Surface* window;
    SDL_Rect texture_rect;
};

bool base_sdl1_init_system(
	struct Base* self
);

bool base_sdl1_init_window(
	struct Base* self,
	const struct BaseConfig* config
);

void base_sdl1_exit(struct Base* self);
void base_sdl1_poll_events(struct Base* self);
void base_sdl1_toggle_fullscreen(struct Base* self);
void base_sdl1_set_window_name(struct Base* self, const char* name);

#ifdef __cplusplus
}
#endif
