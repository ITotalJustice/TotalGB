#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <SDL.h>

bool font_init(const char* font_path, float s_size, float m_size, float l_size);
void font_exit(void);


#ifdef __cplusplus
}
#endif
