#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"

enum GB_BmpBitsPerPixel {
    /* grey */
    GB_BITS_PER_PIXEL_MONOCHROME = 1,
    /* 4bit RGB */
    GB_BITS_PER_PIXEL_4bit = 4,
    /* 8bit RGB */
    GB_BITS_PER_PIXEL_8bit = 8,
    /* 16bit RGB */
    GB_BITS_PER_PIXEL_16bit = 16,
    /* 24bit RGB */
    GB_BITS_PER_PIXEL_24bit = 24,
};

enum GB_BmpCompressionType {
    /* no compression */
    GB_BMP_COMPRESSION_TYPE_BL_RGB,
    /* RLE encoding */
    GB_BMP_COMPRESSION_TYPE_BL_RLE8,
    GB_BMP_COMPRESSION_TYPE_BL_RLE4,
};

struct GB_BmpHeader {
    GB_U16 signature; /* BM */
    GB_U32 file_size;
    GB_U32 reserved; /* = 0 */
    GB_U32 data_offset;
} __attribute__((__packed__));

struct GB_BmpInfoHeader {
    GB_U32 size; /* = 40 */
    GB_S32 width;
    GB_S32 height;
    GB_U16 planes; /* = 1 */
    GB_U16 bits_per_pixel;
    GB_U32 compression;
    GB_U32 image_size;
    GB_S32 xpixels_per_m;
    GB_S32 ypixels_per_m;
    GB_U32 colours_used;
    GB_U32 important_colours;
} __attribute__((__packed__));

struct GB_Bmp {
    struct GB_BmpHeader header;
    struct GB_BmpInfoHeader info_header;
    GB_U16 pixel_data[GB_SCREEN_HEIGHT][GB_SCREEN_WIDTH];
} __attribute__((__packed__));

/* The filled out bmp struct can be written directly to a file and saved with */
/* extension .bmp to be opened by any image reader. */
GB_BOOL GB_screenshot(const struct GB_Core* gb, struct GB_Bmp* bmp);

#ifndef GB_NO_STDIO
GB_BOOL GB_screenshot_to_file(const struct GB_Core* gb, const char* path);
#endif /* GB_NO_STDIO */

#ifdef __cplusplus
}
#endif
