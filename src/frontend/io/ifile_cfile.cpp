#include "ifile_cfile.hpp"

namespace mgb::io {

Cfile::Cfile(const char* path, const char* mode) {
    this->file = fopen(path, mode);
}

Cfile::Cfile(const std::string& path, const std::string& mode) {
    this->file = fopen(path.c_str(), mode.c_str());
}

Cfile::~Cfile() {
    if (this->file) {
        fclose(this->file);
    }
}

auto Cfile::is_open(void) const -> bool {
    return (this->file);
}

auto Cfile::good(void) const -> bool {
    return true;
}

auto Cfile::flush(void) -> bool {
    fflush(this->file);
    return true;
}

auto Cfile::read(std::uint8_t* data, std::uint32_t len) -> bool {
    return this->file && fread(static_cast<void*>(data), static_cast<size_t>(len), 1, this->file) > 0;
}

auto Cfile::write(const std::uint8_t* data, std::uint32_t len) -> bool {
    return this->is_open() && fwrite(static_cast<const void*>(data), len, 1, this->file);
}

auto Cfile::seek(std::uint32_t len, std::uint32_t ed) -> bool {
    return this->file && fseek(this->file, len, ed) == 0;
}

auto Cfile::tell(void) -> std::uint32_t {
    return this->file ? static_cast<std::uint32_t>(ftell(this->file)) : 0;
}

auto Cfile::getFileSize(void) -> std::uint32_t {
    const auto curr = this->tell();
    this->seek(0, SEEK_END);
    this->file_size = this->tell();
    this->seek(curr, SEEK_SET);
    return this->file_size;
}

} // namespace mgb::io
