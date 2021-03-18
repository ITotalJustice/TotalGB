#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>

/* this can fail if the int is bigger than 8 bytes */
static inline size_t vln_read(const uint8_t* data, size_t* offset) {
    size_t result = 0;
    size_t shift = 0;
    uint8_t value = 0;

    /* just in case its a bad patch, only run until max size */
    for (uint8_t i = 0; i < sizeof(size_t); ++i) {

        value = data[*offset];
        ++*offset;

        if (value & 0x80) {
            result += (value & 0x7F) << shift;
            break;
        }

        result += (value | 0x80) << shift;
        shift += 7;
    }

    return result;
}

static inline uint8_t safe_read(
    const uint8_t* restrict data,
    size_t* restrict offset, size_t size
) {
    if (*offset < size) {
        const uint8_t value = data[*offset];
        ++*offset;
        return value;
    }
    
    return 0;
}

static inline void safe_write(
    uint8_t* restrict data, uint8_t value,
    size_t* restrict offset, size_t size
) {
    if (*offset < size) {
        data[*offset] = value;
        ++*offset;
    }
}

#ifdef __cplusplus
}
#endif
