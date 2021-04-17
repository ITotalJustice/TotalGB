#include "frontend/util/io/ifile_gzip.hpp"

#ifndef MGB_NO_GZIP

namespace mgb::io {


Gzip::Gzip(const char* path, const char* mode) {
    this->file = gzopen(path, mode);
}

Gzip::Gzip(const std::string& path, const std::string& mode) {
    this->file = gzopen(path.c_str(), mode.c_str());
}

Gzip::~Gzip() {
    if (this->file) {
        gzclose(file);
    }
}

bool Gzip::is_open(void) const {
    return (this->file) != nullptr;
}

bool Gzip::good(void) const {
    return true;
}

bool Gzip::read(std::uint8_t* data, std::uint32_t len) {
    return this->file && gzread(this->file, data, len) > 0;
}

bool Gzip::write(const std::uint8_t* data, std::uint32_t len) {
    return this->is_open() && gzwrite(this->file, data, len) > 0;
}

bool Gzip::seek(std::uint32_t len, std::uint32_t ed) {
    return this->file && gzseek(this->file, len, ed) != -1;
}

std::uint32_t Gzip::tell(void) {
    return this->file ? static_cast<std::uint32_t>(gztell(this->file)) : 0;
}

bool Gzip::flush(void) {
    gzflush(this->file, 1);
    return true;
}

std::uint32_t Gzip::getFileSize(void) {
    const auto curr = this->tell();
    this->seek(100, 0);
    this->file_size = this->tell();
    this->seek(curr, SEEK_SET);
    return this->file_size;
}

} // namespace io

#endif // #ifndef MGB_NO_GZIP
