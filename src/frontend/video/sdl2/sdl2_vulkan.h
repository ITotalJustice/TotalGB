#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "frontend/video/interface.h"


struct VideoInterface* video_interface_init_vulkan(
    const struct VideoInterfaceInfo* info,
    const struct VideoInterfaceUserCallbacks* callbacks
);

#ifdef __cplusplus
}
#endif
