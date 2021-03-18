#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t crc32(const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif
