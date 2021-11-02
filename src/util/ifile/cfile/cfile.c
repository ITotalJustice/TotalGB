#include "cfile.h"

#include <stdlib.h>

#define PRIVATE_TO_CTX ctx_t* ctx = (ctx_t*)_private

#ifndef HAS_SDL2
    #define HAS_SDL2 0
#endif

// if we are using sdl, then might as well use sdlrwops
// the added benifit here rwops just work (tm) with android assets
#if HAS_SDL2

#include <SDL_rwops.h>

typedef SDL_RWops ctx_t;

static void iclose(void* _private) {
    PRIVATE_TO_CTX;
    SDL_RWclose(ctx);
}

static bool iread(void* _private, void* data, size_t len) {
    PRIVATE_TO_CTX;
    return len == SDL_RWread(ctx, data, 1, len);
}

static bool iwrite(void* _private, const void* data, size_t len) {
    PRIVATE_TO_CTX;
    return len == SDL_RWwrite(ctx, data, 1, len);
}

static bool iseek(void* _private, long offset, int whence) {
    PRIVATE_TO_CTX;
    return -1 != SDL_RWseek(ctx, offset, whence);
}

static size_t itell(void* _private) {
    PRIVATE_TO_CTX;

    const Sint64 result = SDL_RWtell(ctx);

    if (result < 0) {
        return 0;
    }

    return (size_t)result;
}

static size_t isize(void* _private) {
    PRIVATE_TO_CTX;

    const Sint64 file_size = SDL_RWsize(ctx);

    if (file_size < 0) {
        return 0;
    }

    return (size_t)file_size;
}

IFile_t* icfile_open(const char* file, enum IFileMode mode, int flags) {
    IFile_t* ifile = NULL;
    ctx_t* ctx = NULL;

    ifile = malloc(sizeof(IFile_t));

    if (!ifile) {
        goto fail;
    }

    const char* modes[] = {
        [IFileMode_READ] = "rb",
        [IFileMode_WRITE] = "wb",
    };

    ctx = SDL_RWFromFile(file, modes[mode]);

    if (!ctx) {
        goto fail;
    }

    const IFile_t _ifile = {
        ._private = ctx,
        .close  = iclose,
        .read   = iread,
        .write  = iwrite,
        .seek   = iseek,
        .tell   = itell,
        .size   = isize,
    };

    *ifile = _ifile;

    return ifile;

fail:
    if (ifile) {
        free(ifile);
    }

    return NULL;
}

#else

#include <stdio.h>

typedef FILE ctx_t;


static void iclose(void* _private) {
    PRIVATE_TO_CTX;
    fclose(ctx);
}

static bool iread(void* _private, void* data, size_t len) {
    PRIVATE_TO_CTX;
    return len == fread(data, 1, len, ctx);
}

static bool iwrite(void* _private, const void* data, size_t len) {
    PRIVATE_TO_CTX;
    return len == fwrite(data, 1, len, ctx);
}

static bool iseek(void* _private, long offset, int whence) {
    PRIVATE_TO_CTX;
    return 0 == fseek(ctx, offset, whence);
}

static size_t itell(void* _private) {
    PRIVATE_TO_CTX;

    const long result = ftell(ctx);

    if (result < 0) {
        return 0;
    }

    return (size_t)result;
}

static size_t isize(void* _private) {
    PRIVATE_TO_CTX;

    const size_t old_offset = itell(ctx);
    iseek(ctx, 0, SEEK_END);
    const long file_size = itell(ctx);
    iseek(ctx, old_offset, SEEK_SET);

    if (file_size < 0) {
        return 0;
    }

    return (size_t)file_size;
}

IFile_t* icfile_open(const char* file, enum IFileMode mode, int flags) {
    IFile_t* ifile = malloc(sizeof(IFile_t));

    // if this ever fails, we have less than 56 bytes of mem
    if (!ifile) {
        exit(-1);
    }

    const char* modes[] = {
        [IFileMode_READ] = "rb",
        [IFileMode_WRITE] = "wb",
    };

    ctx_t* ctx = fopen(file, modes[mode]);

    if (!ctx) {
        goto fail;
    }

    const IFile_t _ifile = {
        ._private = ctx,
        .close  = iclose,
        .read   = iread,
        .write  = iwrite,
        .seek   = iseek,
        .tell   = itell,
        .size   = isize,
    };

    *ifile = _ifile;

    return ifile;

fail:
    if (ifile) {
        free(ifile);
    }

    return NULL;
}

#endif // HAS_SDL2
