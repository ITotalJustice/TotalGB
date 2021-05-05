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

#include <stdint.h>
#include <stdbool.h>


struct NkInterface {
    void* _private;

    void (*quit)(void* _private);
    bool (*event)(void* _private);
    void (*render)(void* _private, enum nk_anti_aliasing AA);
};

void nk_interface_quit(
    struct NkInterface* self
);

bool nk_interface_handle_event(
    struct NkInterface* self
);

void nk_interface_render(
    struct NkInterface* self,
    enum nk_anti_aliasing AA
);

#ifdef __cplusplus
}
#endif
