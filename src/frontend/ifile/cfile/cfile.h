#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../ifile.h"


IFile_t* icfile_open(const char* file, const char* mode);

#ifdef __cplusplus
}
#endif
