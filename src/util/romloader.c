#include "romloader.h"
#include "util.h"

#include "ifile/cfile/cfile.h"
#include "ifile/zip/zip.h"
#include "ifile/mem/mem.h"


IFile_t* romloader_open(const char* path)
{
    const enum ExtensionType type = util_get_extension_type(path, ExtensionOffsetType_LAST);

    if (type == ExtensionType_ROM)
    {
        return icfile_open(path, IFileMode_READ, 0);
    }
    else if (type == ExtensionType_ZIP)
    {
        return izip_open(path, IFileMode_READ, ExtensionType_ROM);
    }

    return NULL;
}

IFile_t* romloader_open_mem(const char* path, const void* data, size_t size)
{
    const enum ExtensionType type = util_get_extension_type(path, ExtensionOffsetType_LAST);

    if (type == ExtensionType_ROM)
    {
        return imem_open((void*)data, size, IFileMode_READ, 0);
    }
    else if (type == ExtensionType_ZIP)
    {
        return izip_open_mem((void*)data, size, IFileMode_READ, ExtensionType_ROM);
    }

    return NULL;
}
