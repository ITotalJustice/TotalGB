#include "ups.h"
#include "crc32.h"
#include "util.h"

#include <assert.h>

/* header is 4 bytes plus at least 2 bytes for input and output size */
#define PATCH_HEADER_SIZE 0x4
#define PATCH_MIN_SIZE 0x6

int ups_verify_header(const uint8_t* patch, size_t patch_size) {
    assert(patch && patch_size);

    if (!patch || patch_size < PATCH_HEADER_SIZE) {
        return -1;
    }

    /* verify header */
    if (patch[0] != 'U' || patch[1] != 'P' ||
        patch[2] != 'S' || patch[3] != '1') {
        return -1;
    }

    return 0;
}

int ups_get_sizes(
    const uint8_t* patch, size_t patch_size,
    size_t* dst_size, size_t* src_size, size_t* offset
) {
    (void)patch_size; // unused

    /* the offset is after the header */
    size_t offset_local = PATCH_HEADER_SIZE;

    const size_t input_size = vln_read(patch, &offset_local);
    const size_t output_size = vln_read(patch, &offset_local);

    if (src_size != NULL) {
        *src_size = input_size;
    }
    if (dst_size != NULL) {
        *dst_size = output_size;
    }
    if (offset != NULL) {
        *offset = offset_local;
    }

    return 0;
}

/* applies the ups patch to the dst data */
int ups_patch(
    uint8_t* dst, size_t dst_size,
    const uint8_t* src, size_t src_size,
    const uint8_t* patch, size_t patch_size
) {
    assert(dst && dst_size && src && src_size && patch && patch_size);

    if (!dst || !dst_size || !src || !src_size ||
        !patch || patch_size < PATCH_MIN_SIZE) {
        return -1;
    }

    size_t patch_offset = 0;
    size_t input_size = 0;
    size_t output_size = 0;

    if (ups_verify_header(patch, patch_size)) {
        return -1;
    }

    if (ups_get_sizes(patch, patch_size, &input_size, &output_size, &patch_offset)) {
        return -1;
    }

    if (dst_size < output_size) {
        return -1;
    }

    /* crc's are at the last 12 bytes, each 4 bytes each. */
    /* memcpy can also work here, but a simple cast works fine. */
    const uint32_t src_crc = *(uint32_t*)(patch + (patch_size - 12));
    const uint32_t dst_crc = *(uint32_t*)(patch + (patch_size - 8));
    const uint32_t patch_crc = *(uint32_t*)(patch + (patch_size - 4));
    
    /* check that the src and patch is valid. */
    /* dst is checked at the end. */
    const uint32_t src_crc2 = GB_crc32(src, src_size);
    /* we don't check it's own crc32 (obviously) */
    const uint32_t patch_crc2 = GB_crc32(patch, patch_size - 4);

    #define CHECK_CRC(a, b) if (a != b) { return -1; }

    CHECK_CRC(patch_crc, patch_crc2);
    CHECK_CRC(src_crc, src_crc2);

    /* we've read the crc's now, reduce the size. */
    patch_size -= 12;
    
    size_t src_offset = 0;
    size_t dst_offset = 0;

    /* read hunks and patch */
    while (patch_offset < patch_size) {
        assert(patch_offset < patch_size);

        size_t len = vln_read(patch, &patch_offset);

        while (len-- && dst_offset < dst_size) {
            assert(dst_offset < dst_size);

            const uint8_t value = safe_read(src, &src_offset, src_size);
            safe_write(dst, value, &dst_offset, dst_size);
        }

        while (dst_offset < dst_size) {
            assert(patch_offset < patch_size);
            assert(dst_offset < dst_size);

            const uint8_t patch_value = safe_read(patch, &patch_offset, patch_size);
            const uint8_t src_value = safe_read(src, &src_offset, src_size);

            safe_write(dst, src_value ^ patch_value, &dst_offset, dst_size);

            if (patch_value == 0) {
                break;
            }
        }
    }

    /* patch can be smaller than src, in this case keep writing from src */
    while (src_offset < src_size && dst_offset < dst_size) {
        const uint8_t value = safe_read(src, &src_offset, src_size);
        safe_write(dst, value, &dst_offset, dst_size);
    }

    const uint32_t dst_crc2 = GB_crc32(dst, dst_size);
    CHECK_CRC(dst_crc, dst_crc2);

    return 0;
}
