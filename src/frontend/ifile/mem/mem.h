#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../ifile.h"


// this acts as a view of the data, it does NOT free the memory after!
IFile_t* imem_open(void* data, size_t len);

IFile_t* imem_open_own(void* data, size_t len, void(*free_func)(void* data));

// grows the mem as it's written to
IFile_t* imem_open_grow(size_t max_size);


IFile_t* imem_open_copy(const void* data, size_t len);

#ifdef __cplusplus
}
#endif
