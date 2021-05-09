#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../ifile.h"


IFile_t* izip_open(const char* path, enum IFileMode mode);


// returns how many files / folders are in the zip
size_t izip_get_file_count(IFile_t* ifile);

void izip_close_file(IFile_t* ifile);

bool izip_open_file(IFile_t* ifile, const char *name);

// only used for read api!
// if found, the file is opened
bool izip_find_file_callback(IFile_t* ifile, bool(*cmp)(const char*));

#ifdef __cplusplus
}
#endif
