#include "ifile.h"

#include <stdlib.h>


void ifile_close(IFile_t* ifile) {
    if (ifile) {
        ifile->close(ifile->_private);
        free(ifile);
    }
}

bool ifile_read(IFile_t* ifile, void* data, size_t len) {
    return ifile->read(ifile->_private, data, len);
}

bool ifile_write(IFile_t* ifile, const void* data, size_t len) {
    return ifile->write(ifile->_private, data, len);
}

bool ifile_seek(IFile_t* ifile, long offset, int whence) {
    return ifile->seek(ifile->_private, offset, whence);
}

size_t ifile_tell(IFile_t* ifile) {
    return ifile->tell(ifile->_private);
}

size_t ifile_size(IFile_t* ifile) {
    return ifile->size(ifile->_private);
}
