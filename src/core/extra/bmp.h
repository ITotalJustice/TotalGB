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
    uint16_t signature; /* BM */
    uint32_t file_size;
    uint32_t reserved; /* = 0 */
    uint32_t data_offset;
} __attribute__((__packed__));

struct GB_BmpInfoHeader {
    uint32_t size; /* = 40 */
    int32_t width;
    int32_t height;
    uint16_t planes; /* = 1 */
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t xpixels_per_m;
    int32_t ypixels_per_m;
    uint32_t colours_used;
    uint32_t important_colours;
} __attribute__((__packed__));

struct GB_Bmp {
    struct GB_BmpHeader header;
    struct GB_BmpInfoHeader info_header;
    uint16_t pixel_data[GB_SCREEN_HEIGHT][GB_SCREEN_WIDTH];
} __attribute__((__packed__));

/* The filled out bmp struct can be written directly to a file and saved with */
/* extension .bmp to be opened by any image reader. */
bool GB_screenshot(const struct GB_Core* gb, struct GB_Bmp* bmp);

#ifndef GB_NO_STDIO
bool GB_screenshot_to_file(const struct GB_Core* gb, const char* path);
#endif /* GB_NO_STDIO */

#ifdef __cplusplus
}
#endif
