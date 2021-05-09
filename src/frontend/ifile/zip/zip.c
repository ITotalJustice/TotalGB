#include "zip.h"

#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <minizip/ioapi_mem.h>


typedef struct {
    union {
        unzFile u;
        zipFile z;
    } file;
    ourmemory_t ourmem;
    enum IFileMode mode;
} ctx_t;

#define PRIVATE_TO_CTX ctx_t* ctx = (ctx_t*)_private
#define IFILE_TO_CTX ctx_t* ctx = (ctx_t*)ifile->_private


static void internal_close(void* _private) {
    PRIVATE_TO_CTX;

    if (ctx) {
        switch (ctx->mode) {
            case IFileMode_READ:
                if (ctx->file.u) {
                    unzCloseCurrentFile(ctx->file.u);
                    unzClose(ctx->file.u);
                    ctx->file.u = NULL;
                }
                break;

            case IFileMode_WRITE:
                if (ctx->file.z) {
                    zipCloseFileInZip(ctx->file.z);
                    zipClose(ctx->file.z, NULL);
                    ctx->file.z = NULL;
                }
                break;

            case IFileMode_APPEND:
                break; // not supported (yet)
        }

        if (ctx->ourmem.base) {
            free(ctx->ourmem.base);
            ctx->ourmem.base = NULL;
        }

        free(ctx);
        ctx = NULL;
    }
}

static bool internal_read(void* _private, void* data, size_t len) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    return 0 < unzReadCurrentFile(ctx->file.u, data, len);
}

static bool internal_write(void* _private, const void* data, size_t len) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_WRITE) {
        return false;
    }

    return 0 < zipWriteInFileInZip(ctx->file.z, data, len);
}

static bool internal_seek(void* _private, long offset, int whence) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    // cannot seek backwards!
    if (offset < 0) {
        return false;
    }

    return UNZ_OK == unzSeek(ctx->file.u, (uint32_t)offset, whence);
}

static size_t internal_tell(void* _private) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    // returns uncompressed offset
    return unzTell(ctx->file.u);
}

static size_t internal_size(void* _private) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    unz_file_info info = {0};

    const int result = unzGetCurrentFileInfo(
        ctx->file.u, &info,
        NULL, 0, NULL, 0, NULL, 0
    );

    if (result != UNZ_OK) {
        return 0;
    }

    return info.uncompressed_size;
}

static IFile_t* open_read(const char* path) {
    IFile_t* ifile = NULL;
    ctx_t* ctx = NULL;
    unzFile file = NULL;

    ifile = malloc(sizeof(IFile_t));
    if (!ifile) {
        goto fail;
    }

    ctx = malloc(sizeof(ctx_t));
    if (!ctx) {
        goto fail;
    }

    file = unzOpen(path);
    if (!file) {
        goto fail;
    }

    const ctx_t _ctx = {
        .file.u = file,
        .ourmem = {0}, // unused!
        .mode = IFileMode_READ
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

fail:
    if (ifile) {
        free(ifile);
        ifile = NULL;
    }

    if (ctx) {
        free(ctx);
        ctx = NULL;
    }

    if (file) {
        unzClose(file);
        file = NULL;
    }

    return NULL;
}

static IFile_t* open_write(const char* path) {
    IFile_t* ifile = NULL;
    ctx_t* ctx = NULL;
    unzFile file = NULL;

    ifile = malloc(sizeof(IFile_t));
    if (!ifile) {
        goto fail;
    }

    ctx = malloc(sizeof(ctx_t));
    if (!ctx) {
        goto fail;
    }

    file = zipOpen(path, APPEND_STATUS_CREATE);
    if (!file) {
        goto fail;
    }

    const ctx_t _ctx = {
        .file.z = file,
        .ourmem = {0}, // unused!
        .mode = IFileMode_WRITE
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

fail:
    if (ifile) {
        free(ifile);
        ifile = NULL;
    }

    if (ctx) {
        free(ctx);
        ctx = NULL;
    }

    if (file) {
        unzClose(file);
        file = NULL;
    }

    return NULL;
}

IFile_t* izip_open(const char* path, enum IFileMode mode) {
    switch (mode) {
        case IFileMode_READ:
            return open_read(path);

        case IFileMode_WRITE:
            return open_write(path);

        case IFileMode_APPEND:
            return NULL; // not supported (yet)
    }

    return NULL;
}

// returns how many files / folders are in the zip
size_t izip_get_file_count(IFile_t* ifile) {
    IFILE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return 0;
    }

    unz_global_info info = {0};

    if (UNZ_OK == unzGetGlobalInfo(ctx->file.u, &info)) {
        return info.number_entry;
    }

    return 0;
}

void izip_close_file(IFile_t* ifile) {
    IFILE_TO_CTX;

    switch (ctx->mode) {
        case IFileMode_READ:
            unzCloseCurrentFile(ctx->file.u);
            break;

        case IFileMode_WRITE:
            zipCloseFileInZip(ctx->file.z);
            break;

        case IFileMode_APPEND:
            break; // not supported (yet)
    }
}

bool izip_open_file(IFile_t* ifile, const char *name) {
    IFILE_TO_CTX;

    switch (ctx->mode) {
        case IFileMode_READ:
            if (UNZ_OK == unzLocateFile(
                ctx->file.u, name, NULL
            )) {
                return UNZ_OK == unzOpenCurrentFile(ctx->file.u);
            }
            return false;

        case IFileMode_WRITE:
            return Z_OK == zipOpenNewFileInZip(
                ctx->file.z, name,
                NULL, NULL, 0, NULL, 0, NULL,
                Z_DEFLATED, Z_DEFAULT_COMPRESSION
            );

        case IFileMode_APPEND:
            return false; // not supported (yet)
    }

    return false;
}

typedef bool(*user_cmp_t)(const char* name);

struct Hacky {
    char _hack; // this will be 0 (NULL)
    user_cmp_t cmp;
};

static int comp(unzFile ufile, const char* f1, const char* f2) {
    (void)ufile;

    // yes, this is hacky, it's the only way to pass user data...
    const struct Hacky* hacky = (const struct Hacky*)f2;
    // the result expects 0 to be yes, and anything else false
    return !hacky->cmp(f1);
}

bool izip_find_file_callback(IFile_t* ifile, user_cmp_t cmp) {
    IFILE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    // this is used as mz calls strlen on the passed in name, so
    // we need to be NULL somewhere.
    struct Hacky hacky = {
        ._hack = '\0',
        .cmp = cmp
    };

    if (Z_OK != unzLocateFile(
        ctx->file.u, (const char*)&hacky, comp
    )) {
        return false;
    }

    return UNZ_OK == unzOpenCurrentFile(ctx->file.u);
}
