#include "core/extra/bmp.h"
#include "core/internal.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


static uint32_t GB_bmp_get_colours_used(enum GB_BmpBitsPerPixel type) {
    switch (type) {
        case GB_BITS_PER_PIXEL_MONOCHROME:
            return 1U;
        case GB_BITS_PER_PIXEL_4bit:
            return 16U; /* 2^4 */
        case GB_BITS_PER_PIXEL_8bit:
            return 256U; /* 2^8 */
        case GB_BITS_PER_PIXEL_16bit:
            return 65536U; /* 2^16 */
        case GB_BITS_PER_PIXEL_24bit:
            return 16777216U; /* 2^24 */
    }

    GB_UNREACHABLE(0xFF);
}

static void GB_bmp_fill_header(struct GB_BmpHeader* header) {
    header->signature = 0x4D42; /* 'BM' */;
    header->file_size = sizeof(struct GB_Bmp);
    header->data_offset = offsetof(struct GB_Bmp, pixel_data); /* 54 */
}

static void GB_bmp_fill_info_header(struct GB_BmpInfoHeader* info_header) {
    info_header->size = sizeof(struct GB_BmpInfoHeader); /* 40 */
    info_header->width = GB_SCREEN_WIDTH;
    info_header->height = -GB_SCREEN_HEIGHT; /* negative to flip image */
    info_header->planes = 1;
    info_header->bits_per_pixel = GB_BITS_PER_PIXEL_16bit;
    info_header->compression = GB_BMP_COMPRESSION_TYPE_BL_RGB; /* uncompressed */
    info_header->image_size = GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT * 2; /* optional? */
    info_header->colours_used = GB_bmp_get_colours_used(GB_BITS_PER_PIXEL_16bit);
    info_header->important_colours = 0; /* all */
}

#ifndef GB_BGR555_TO_RGB555
#define GB_BGR555_TO_RGB555(pixel) (((pixel & 0x1F) << 10) | (pixel & 0x3E0) | ((pixel >> 10) & 0x1F))
#endif /* GB_BGR555_TO_RGB555 */

/* have to accept struct as param else compiler will warn that the member pointer */
/* might be unaligned */
static void GB_bmp_fill_pixel_data(const struct GB_Core* gb, struct GB_Bmp* bmp) {
    /* pixels are reversed when stored in bmp, such that rgb will be stored as bgr */
    /* however, this reverse is undone by bmp parsers. */
    /* this means that our bgr555 will be stored as bgr565, but read back as rgb555. */
    /* because of this, we need to flip the red and blue. */

    for (int y = 0; y < GB_SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < GB_SCREEN_WIDTH; ++x) {
            bmp->pixel_data[y][x] = GB_BGR555_TO_RGB555(gb->ppu.pixles[y][x]);
        }
    }
}

bool GB_screenshot(const struct GB_Core* gb, struct GB_Bmp* bmp) {
    assert(gb && bmp);
    
    if (!gb || !bmp) {
        return false;
    }

    memset(bmp, 0, sizeof(struct GB_Bmp));

    /* setup the header */
    GB_bmp_fill_header(&bmp->header);

    /* setup info header */
    GB_bmp_fill_info_header(&bmp->info_header);

    /* copy the pixel data */
    GB_bmp_fill_pixel_data(gb, bmp);

    return true;
}

bool GB_screenshot_to_file(const struct GB_Core* gb, const char* path) {
    assert(gb && path);

    if (!gb || !path) {
        return false;
    }

    FILE* file = fopen(path, "wb");
    if (!file) {
        printf("failed to open\n");
        return false;
    }

    struct GB_Bmp bmp = {0};

    GB_bmp_fill_header(&bmp.header);
    if (1 != fwrite(&bmp.header, sizeof(bmp.header), 1, file)) {
        printf("failed to write header\n");
        goto fail_close;
    }

    GB_bmp_fill_info_header(&bmp.info_header);
    if (1 != fwrite(&bmp.info_header, sizeof(bmp.info_header), 1, file)) {
        printf("failed to write info header\n");
        goto fail_close;
    }

    GB_bmp_fill_pixel_data(gb, &bmp);
    if (1 != fwrite(bmp.pixel_data, sizeof(bmp.pixel_data), 1, file)) {
        printf("failed to write pixels\n");
        goto fail_close;
    }

    fclose(file);
    return true;

fail_close:
    fclose(file);
    return false;
}
