// this class is used for all core IO.
// it is a pure class, so it is supposed to be inhertied from
// this is handled on the frontend-side.
// romloading, savestates, saves...

#pragma once

#include <cstdint>

namespace mgb::io {

class IFile {
public:
    virtual ~IFile() = default;
    virtual auto read(std::uint8_t* data, std::uint32_t len) -> bool = 0;
    virtual auto write(const std::uint8_t* data, std::uint32_t len) -> bool = 0;
    virtual auto seek(std::uint32_t len, std::uint32_t ed) -> bool = 0;
    virtual auto flush(void) -> bool = 0;
    virtual auto tell(void) -> std::uint32_t = 0;
    virtual auto good(void) const -> bool = 0;
    virtual auto is_open(void) const -> bool = 0;
    auto size(void) {
        return this->file_size ? this->file_size: this->getFileSize();
    }

protected:
    std::uint32_t file_size{0};
    virtual auto getFileSize(void) -> std::uint32_t = 0;
};

} // namespace mgb::io 
