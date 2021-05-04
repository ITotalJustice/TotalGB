#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../interface.h"
#include <SDL2/SDL.h>


struct BaseEventCallbacks {
	void* user;
	void (*on_resize)(void* user, int w, int h);
};

struct BaseConfig {
	struct BaseEventCallbacks event_callbacks;

	const char* const window_name;
	int window_flags;
	int x, y, w, h;
	
	int min_win_w;
	int min_win_h;

	int max_win_w;
	int max_win_h;

	bool set_min_win;
	bool set_max_win;
};

struct Base {
    struct VideoInterfaceUserCallbacks callbacks;
    SDL_Window* window;
    SDL_Rect texture_rect;
};

bool base_sdl2_init_system(
	struct Base* self
);

bool base_sdl2_init_window(
	struct Base* self,
	const struct BaseConfig* config,
    const struct VideoInterfaceUserCallbacks* callbacks
);

void base_sdl2_exit(struct Base* self);
void base_sdl2_poll_events(struct Base* self);
void base_sdl2_toggle_fullscreen(struct Base* self);
void base_sdl2_set_window_name(struct Base* self, const char* name);

#ifdef __cplusplus
}
#endif
