#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../ifile.h"


IFile_t* icfile_open(const char* file, enum IFileMode mode);

#ifdef __cplusplus
}
#endif
