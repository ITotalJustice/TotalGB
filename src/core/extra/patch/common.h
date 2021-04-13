#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>


/* simple stream structs */
struct PatchStreamR {
    void* user;
    size_t size; /* must be filled */
    size_t (*_read)(uint8_t*, size_t, size_t, void*);
    int (*_seek)(void*, long, int);
};

struct PatchStreamW {
    void* user;
    size_t size; /* must be filled */
    size_t (*_write)(const uint8_t*, size_t, size_t, void*);
    int (*_seek)(void*, long, int);
};

struct PatchStreamRW {
    void* user;
    size_t size; /* must be filled */
    size_t (*_read)(uint8_t*, size_t, size_t, void*);
    size_t (*_write)(const uint8_t*, size_t, size_t, void*);
    int (*_seek)(void*, long, int);
};

#ifdef __cplusplus
}
#endif
