#include "bps.h"
#include "crc32.h"
#include "util.h"

#include <stdio.h>
#include <assert.h>

/* header: 4 */
/* sizes_min: 2 */
/* crc's: 12 */
#define BPS_MIN_SIZE 18

int bps_verify_header(const uint8_t* patch, size_t patch_size) {
    assert(patch && patch_size);

    if (!patch || patch_size < 4) {
        return -1;
    }

    /* verify header */
    if (patch[0] != 'B' || patch[1] != 'P' ||
        patch[2] != 'S' || patch[3] != '1') {
        return -1;
    }

    return 0;
}

int bps_get_sizes(
    const uint8_t* patch, size_t patch_size,
    size_t* dst_size, size_t* src_size,
    size_t* meta_size, size_t* offset
) {
    assert(patch && patch_size);

    if (!patch || !patch_size) {
        return -1;
    }

    /* the offset is after the header */
    size_t offset_local = 4;

    const size_t input_size = vln_read(patch, &offset_local);
    const size_t output_size = vln_read(patch, &offset_local);
    const size_t metadata_size = vln_read(patch, &offset_local);

    if (src_size != NULL) {
        *src_size = input_size;
    }
    if (dst_size != NULL) {
        *dst_size = output_size;
    }
    if (meta_size != NULL) {
        *meta_size = metadata_size;
    }
    if (offset != NULL) {
        *offset = offset_local;
    }

    return 0;
}

/* dst_size: large enough to fit entire output */
int bps_patch(
    uint8_t* dst, size_t dst_size,
    const uint8_t* src, size_t src_size,
    const uint8_t* patch, size_t patch_size
) {
    assert(dst && dst_size && src && src_size && patch && patch_size);

    if (!dst || !dst_size || !src || !src_size ||
        !patch || patch_size < BPS_MIN_SIZE) {
        return -1;
    }

    size_t patch_offset = 0;
    size_t input_size = 0;
    size_t output_size = 0;
    size_t meta_size = 0;

    if (bps_verify_header(patch, patch_size)) {
        return -1;
    }

    if (bps_get_sizes(patch, patch_size,
        &output_size, &input_size, &meta_size, &patch_offset)) {
        return -1;
    }

    /* crc's are at the last 12 bytes, each 4 bytes each. */
    /* memcpy can also work here, but a simple cast works fine. */
    const uint32_t src_crc = *(uint32_t*)(patch + (patch_size - 12));
    const uint32_t dst_crc = *(uint32_t*)(patch + (patch_size - 8));
    const uint32_t patch_crc = *(uint32_t*)(patch + (patch_size - 4));

    /* we've read the crc's now, reduce the size. */
    patch_size -= 12;
    
    /* skip over metadata (if any) */
    patch_offset += meta_size;

    /* check that the src and patch is valid. */
    /* dst is checked at the end. */
    const uint32_t src_crc2 = GB_crc32(src, src_size);
    const uint32_t patch_crc2 = GB_crc32(patch, patch_size);

    printf("[BPS] src_crc: %u\n", src_crc);
    printf("[BPS] dst_crc: %u\n", dst_crc);
    printf("[BPS] patch_crc: %u\n", patch_crc);

    printf("[BPS] src_crc2: %u\n", src_crc2);
    printf("[BPS] patch_crc2: %u\n", patch_crc2);

    /*
    As such, offsets are encoded relatively to the current positions. These offsets
    can move the read cursors forward or backward. To support negative numbers with
    variable-integer encoding requires us to store the negative flag as the lowest
    bit, followed by the absolute value (eg abs(-1) = 1) */


    return 0;
}
