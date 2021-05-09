#include "romloader.h"
#include "util.h"

#include "ifile/cfile/cfile.h"
#include "ifile/zip/zip.h"
#include "ifile/gzip/gzip.h"
#include "ifile/mem/mem.h"


static bool zip_cmp_func(const char* name) {
    return util_is_extension(
        name, ExtensionOffsetType_LAST, ExtensionType_ROM
    );
}

static IFile_t* load_from_cfile(const char* path) {
    return icfile_open(path, IFileMode_READ);
}

static IFile_t* load_from_zip(const char* path) {
    IFile_t* file = izip_open(path, IFileMode_READ);

    if (!file) {
        goto fail;
    }

    if (!izip_find_file_callback(file, zip_cmp_func)) {
        goto fail;
    }

    return file;

fail:
    if (file) {
        ifile_close(file);
    }

    return NULL;
}

// static IFile_t* load_from_gzip(const char* path) {
    // return igzip_open(path, "rb");
// }

IFile_t* romloader_open(const char* path) {
    const enum ExtensionType type = util_get_extension_type(
        path, ExtensionOffsetType_LAST
    );

    if (type == ExtensionType_ROM) {
        return load_from_cfile(path);
    }

    if (type == ExtensionType_ZIP) {
        return load_from_zip(path);
    }

    // if (type == ExtensionType_GZIP) {
        // return load_from_gzip(path);
    // }

    return NULL;
}
