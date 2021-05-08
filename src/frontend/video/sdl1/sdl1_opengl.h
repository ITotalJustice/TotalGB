#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "frontend/video/interface.h"


struct VideoInterface* video_interface_init_sdl1_opengl(
    const struct VideoInterfaceInfo* info,
    const struct VideoInterfaceUserCallbacks* callbacks
);

#ifdef __cplusplus
}
#endif
