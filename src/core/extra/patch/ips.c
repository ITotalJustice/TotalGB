/* SOURCE: https://zerosoft.zophar.net/ips.php */
#include "ips.h"
#include "util.h"

#include <assert.h>

/* header is 5 bytes plus at least 2 bytes for input and output size */
#define PATCH_HEADER_SIZE 0x5
#define PATCH_MIN_SIZE 0x9
#define PATCH_MAX_SIZE 0x1000000 /* 16MiB */

#define EOF_MAGIC 0x454F46
#define RLE_ENCODING 0

int ips_verify_header(const uint8_t* patch, size_t patch_size) {
    assert(patch && patch_size);

    if (!patch || patch_size < PATCH_HEADER_SIZE) {
        return -1;
    }

    /* verify header */
    if (patch[0] != 'P' || patch[1] != 'A' ||
        patch[2] != 'T' || patch[3] != 'C' || patch[4] != 'H') {
        return -1;
    }

    return 0;
}

/* basically a memcpy */
static uint16_t safe_read2(
    const uint8_t* data,
    size_t* offset, size_t size
) {
    *offset += 2;
    if (*offset > size) {
        return 0;
    }
    return (data[*offset - 2] << 8) | (data[*offset - 1]);
}

static uint32_t safe_read3(
    const uint8_t* data,
    size_t* offset, size_t size
) {
    *offset += 3;
    if (*offset > size) {
        return 0;
    }
    return (data[*offset - 3] << 16) | (data[*offset - 2] << 8) | (data[*offset - 1]);
}

/* applies the ups patch to the dst data */
int ips_patch(
    uint8_t* dst, size_t dst_size,
    const uint8_t* src, size_t src_size,
    const uint8_t* patch, size_t patch_size
) {
    assert(dst && dst_size && src && src_size && patch && patch_size);
    assert(dst_size >= src_size);
    assert(patch_size > PATCH_MIN_SIZE && patch_size < PATCH_MAX_SIZE);

    if (!dst || !dst_size || !src || !src_size || !patch) {
        return -1;
    }

    if (dst_size < src_size) {
        return -1;
    }

    if (patch_size < PATCH_MIN_SIZE || patch_size > PATCH_MAX_SIZE) {
        return -1;
    }

    size_t patch_offset = 0;

    if (ips_verify_header(patch, patch_size)) {
        return -1;
    }

    #define ASSERT_BOUNDS(offset, size) assert(offset < size); if (offset >= size) { return -1; }

    patch_offset = PATCH_HEADER_SIZE;

    while (patch_offset < patch_size) {
        size_t offset = safe_read3(patch, &patch_offset, patch_size);
        /* check if last 3 bytes were EOF */
        if (offset == EOF_MAGIC) {
            break;
        }

        uint16_t size = safe_read2(patch, &patch_offset, patch_size);

        if (size == RLE_ENCODING) {
            uint16_t rle_size = safe_read2(patch, &patch_offset, patch_size);
            uint8_t value = safe_read(patch, &patch_offset, patch_size);

            while (rle_size--) {
                ASSERT_BOUNDS(offset, dst_size);
                ASSERT_BOUNDS(patch_offset, patch_size);

                safe_write(dst, value, &offset, dst_size);
            }
        } else {
            while (size--) {
                ASSERT_BOUNDS(offset, dst_size);
                ASSERT_BOUNDS(patch_offset, patch_size);

                uint8_t value = safe_read(patch, &patch_offset, patch_size);
                safe_write(dst, value, &offset, dst_size);
            }
        }
    }

    return 0;
}
