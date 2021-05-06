#pragma once

// TJ:
// this is a slightly modified version of nk_sdl_gl2.h
// the goal is keep it generic enough so that it works with
// all backends, as long as they are gl2.
// i plan to do this with all demo headers from the repo, so that
// i'll have gl3, gl4, sdl_surface etc...

#ifdef __cplusplus
extern "C" {
#endif

#include "nuklear/defines.h"
#include "nuklear/nuklear.h"

#include "frontend/video/interface.h"

#include <stdint.h>
#include <stdbool.h>


struct NkInterfaceInitConfig {
    // the actual size of the window
    int window_w, window_h;
    // the gl viewport size
    int viewport_w, viewport_h;
};

struct NkInterface {
    void* _private;

    struct nk_context* (*get_context)(void* _private);
    void (*quit)(void* _private);
    void (*set_window_size)(void* _private, int w, int h);
    void (*set_viewport_size)(void* _private, int w, int h);
    void (*render)(void* _private, enum nk_anti_aliasing AA);
};

struct nk_context* nk_interface_get_context(
	struct NkInterface* self
);

void nk_interface_quit(
    struct NkInterface* self
);

void nk_interface_set_window_size(
    struct NkInterface* self, int w, int h
);

void nk_interface_set_viewport_size(
    struct NkInterface* self, int w, int h
);

void nk_interface_input_begin(
	struct NkInterface* self
);

void nk_interface_input_end(
	struct NkInterface* self
);

bool nk_interface_on_mouse_button(
    struct NkInterface* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
);

bool nk_interface_on_mouse_motion(
    struct NkInterface* self,
    int x, int y, int xrel, int yrel
);

bool nk_interface_on_key(
    struct NkInterface* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
);

bool nk_interface_on_button(
    struct NkInterface* self,
    enum VideoInterfaceButton button, bool down
);

bool nk_interface_on_axis(
    struct NkInterface* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);

void nk_interface_render(
    struct NkInterface* self,
    enum nk_anti_aliasing AA
);

#ifdef __cplusplus
}
#endif
