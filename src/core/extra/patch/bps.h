#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int bps_verify_header(const uint8_t* patch, size_t patch_size);

/* dst_size: [optional] */
/* src_size: [optional] */
/* metadata_size: [optional] */
/* offset: [optional] */
int bps_get_sizes(
    const uint8_t* patch, size_t patch_size,
    size_t* dst_size, size_t* src_size, 
    size_t* meta_size, size_t* offset
);

/* dst_size: large enough to fit entire output */
int bps_patch(
    uint8_t* dst, size_t dst_size,
    const uint8_t* src, size_t src_size,
    const uint8_t* patch, size_t patch_size
);

#ifdef __cplusplus
}
#endif
