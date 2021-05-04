#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ifile/ifile.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


IFile_t* romloader_open(const char* path);

#ifdef __cplusplus
}
#endif
