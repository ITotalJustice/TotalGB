#include "frontend/util/io/ifile_mem.hpp"

#include <cstring>


namespace mgb::io {


MemFile::MemFile(std::vector<std::uint8_t>&& _data)
: data{std::move(_data)} {

}

bool MemFile::is_open(void) const {
    return !this->data.empty();
}

bool MemFile::good(void) const {
    return true;
}

bool MemFile::flush(void) {
    return true;
}

bool MemFile::read(std::uint8_t* _data, std::uint32_t len) {
    if ((this->tell() + len) > this->size()) {
        return false;
    }
    std::memcpy(static_cast<void*>(_data), static_cast<const void*>(this->data.data() + this->tell()), static_cast<size_t>(len));
    return true;
}

bool MemFile::write(const std::uint8_t* _data, std::uint32_t len) {
    if ((this->tell() + len) > this->size()) {
        return false;
    }
    std::memcpy(static_cast<void*>(this->data.data() + this->tell()), static_cast<const void*>(_data), static_cast<size_t>(len));
    return true;
}

bool MemFile::seek(std::uint32_t len, std::uint32_t ed) {
    switch (ed) {
        case 0: this->pos = len; return true;
        case 1: this->pos += len; return true;
        case 2: this->pos = this->data.size(); return true;
        default: return false;
    }
}

std::uint32_t MemFile::tell(void) {
    return this->pos;
}

std::uint32_t MemFile::getFileSize(void) {
    return this->data.size();
}

} // namespace mgb::io
