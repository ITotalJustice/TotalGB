#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "frontend/video/interface.h"


struct VideoInterface* video_interface_init_sdl2_opengl(
    const struct VideoInterfaceInfo* info,
    void* user, void (*on_event)(void*, const union VideoInterfaceEvent*)
);

#ifdef __cplusplus
}
#endif
