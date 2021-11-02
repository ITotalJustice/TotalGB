#include "zip.h"
#include "../../util.h"

#include <zip.h>
#include <unzip.h>
#include <ioapi_mem.h>


typedef struct {
    union {
        unzFile u;
        zipFile z;
    } file;
    ourmemory_t* ourmem;
    enum IFileMode mode;
} ctx_t;

#define PRIVATE_TO_CTX ctx_t* ctx = (ctx_t*)_private
#define IFILE_TO_CTX ctx_t* ctx = (ctx_t*)ifile->_private


struct Hacky {
    char _hack; // this will be 0 (NULL)
    int flags;
};

static int comp(unzFile ufile, const char* f1, const char* f2) {
    (void)ufile;

    // yes, this is hacky, it's the only way to pass user data...
    const struct Hacky* hacky = (const struct Hacky*)f2;
    // the result expects 0 to be yes, and anything else false
    const enum ExtensionType type = util_get_extension_type(f1, ExtensionOffsetType_LAST);
    printf("found: %s\n", f1);
    return !(type & hacky->flags);
}

static bool find_and_open_file_type(IFile_t* ifile, int flags) {
    IFILE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    // this is used as mz calls strlen on the passed in name, so
    // we need to be NULL somewhere.
    struct Hacky hacky = {
        ._hack = '\0',
        .flags = flags
    };

    if (Z_OK != unzLocateFile(ctx->file.u, (const char*)&hacky, comp)) {
        printf("failed to locate rom\n");
        return false;
    }

    return UNZ_OK == unzOpenCurrentFile(ctx->file.u);
}

static void iclose(void* _private) {
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
        }

        if (ctx->ourmem) {
            if (ctx->ourmem->base) {
                free(ctx->ourmem->base);
                ctx->ourmem->base = NULL;
            }
            free(ctx->ourmem);
            ctx->ourmem = NULL;
        }

        free(ctx);
        ctx = NULL;
    }
}

static bool iread(void* _private, void* data, size_t len) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return false;
    }

    return 0 < unzReadCurrentFile(ctx->file.u, data, len);
}

static bool iwrite(void* _private, const void* data, size_t len) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_WRITE) {
        return false;
    }

    return 0 < zipWriteInFileInZip(ctx->file.z, data, len);
}

static bool iseek(void* _private, long offset, int whence) {
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

static size_t itell(void* _private) {
    PRIVATE_TO_CTX;

    if (ctx->mode != IFileMode_READ) {
        return 0;
    }

    // returns uncompressed offset
    const int32_t r = unzTell(ctx->file.u);
    return r < 0 ? 0 : r;
}

static size_t isize(void* _private) {
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

static IFile_t* open_read(const char* path, int flags) {
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
        .ourmem = NULL, // unused!
        .mode = IFileMode_READ
    };

    *ctx = _ctx;

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

    if (!find_and_open_file_type(ifile, flags)) {
        goto fail;
    }

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

static IFile_t* open_write(const char* path, int flags) {
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
        .ourmem = NULL, // unused!
        .mode = IFileMode_WRITE
    };

    *ctx = _ctx;

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

IFile_t* izip_open(const char* path, enum IFileMode mode, int flags) {
    switch (mode) {
        case IFileMode_READ:
            return open_read(path, flags);

        case IFileMode_WRITE:
            return open_write(path, flags);
    }

    return NULL;
}

static IFile_t* open_read_mem(void* data, size_t size, int flags) {
    IFile_t* ifile = NULL;
    ctx_t* ctx = NULL;
    unzFile file = NULL;
    ourmemory_t* ourmemory = NULL;
    zlib_filefunc_def filefunc32 = {0};

    ifile = malloc(sizeof(IFile_t));
    if (!ifile) {
        goto fail;
    }

    ctx = malloc(sizeof(ctx_t));
    if (!ctx) {
        goto fail;
    }

    ourmemory = malloc(sizeof(ourmemory_t));
    if (!ourmemory) {
        goto fail;
    }

    ourmemory->size = size;
    ourmemory->base = (char*)data;
    fill_memory_filefunc(&filefunc32, ourmemory);

    file = unzOpen2("__notused__", &filefunc32);
    if (!file) {
        goto fail;
    }

    const ctx_t _ctx = {
        .file.u = file,
        .ourmem = ourmemory,
        .mode = IFileMode_READ
    };

    *ctx = _ctx;

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

    if (!find_and_open_file_type(ifile, flags)) {
        printf("failed to find rom!\n");
        goto fail;
    }

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

    if (ourmemory) {
        free(ourmemory);
        ourmemory = NULL;
    }

    if (file) {
        unzClose(file);
        file = NULL;
    }

    return NULL;
}

IFile_t* izip_open_mem(void* data, size_t size, enum IFileMode mode, int flags) {
    switch (mode) {
        case IFileMode_READ:
            return open_read_mem(data, size, flags);

        case IFileMode_WRITE:
            return NULL;
    }

    return NULL;
}
