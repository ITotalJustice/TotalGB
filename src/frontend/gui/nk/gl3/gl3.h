#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../interface.h"


struct NkInterface* nk_interface_gl3_init(
    struct nk_context** nk_context_out
);

#ifdef __cplusplus
}
#endif
