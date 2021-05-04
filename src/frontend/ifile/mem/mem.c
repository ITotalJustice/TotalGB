#include "mem.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
    void (*free)(void* data);
    uint8_t* data;
    size_t offset;
    size_t size;
    size_t max_size;
} ctx_t;

#define PRIVATE_TO_CTX ctx_t* ctx = (ctx_t*)_private


static void internal_close(void* _private) {
    PRIVATE_TO_CTX;

    if (ctx->data && ctx->free) {
        ctx->free(ctx->data);
        ctx->data = NULL;
        ctx->free = NULL;
    }
}

static bool internal_read(void* _private, void* data, size_t len) {
    PRIVATE_TO_CTX;

    if (ctx->offset + len > ctx->size) {
        return false;
    }

    memcpy(
        data, ctx->data + ctx->offset, len
    );

    ctx->offset += len;

    return true;
}

static bool internal_write(void* _private, const void* data, size_t len) {
    PRIVATE_TO_CTX;

    // todo: handle memory growth if supported!s
    if (ctx->offset + len > ctx->size) {
        return false;
    }

    memcpy(
        ctx->data + ctx->offset, data, len
    );

    ctx->offset += len;

    return true;
}

static bool internal_seek(void* _private, long offset, int whence) {
    PRIVATE_TO_CTX;

    switch (whence) {
        case 0:
            ctx->offset = offset;
            return true;

        case 1:
            ctx->offset += offset;
            return true;

        case 2:
            ctx->offset = ctx->size + offset;
            return true;
    }

    return false;
}

static size_t internal_tell(void* _private) {
    PRIVATE_TO_CTX;

    return ctx->offset;
}

static size_t internal_size(void* _private) {
    PRIVATE_TO_CTX;

    return ctx->size;
}

IFile_t* imem_open_own(
    void* data, size_t len, void(*free_func)(void* data)
) {
    if (!data || !len || !free_func) {
        return NULL;
    }

    IFile_t* ifile = malloc(sizeof(IFile_t));
    ctx_t* ctx = malloc(sizeof(ctx_t));

    const ctx_t _ctx = {
        .free = free_func,
        .data = (uint8_t*)data,
        .offset = 0,
        .size = len,
        .max_size = len,
    };

    *ctx = _ctx;

    const IFile_t _ifile = {
        ._private = ctx,
        .close  = internal_close,
        .read   = internal_read,
        .write  = internal_write,
        .seek   = internal_seek,
        .tell   = internal_tell,
        .size   = internal_size,
    };

    *ifile = _ifile;

    return ifile;
}
