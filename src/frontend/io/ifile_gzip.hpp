#pragma once

#ifndef MGB_NO_GZIP

#include "ifile_base.hpp"

#include <zlib/zlib.h>
#include <string>

namespace mgb::io {

class Gzip final : public IFile {
public:
    Gzip(const char* path, const char* mode);
    Gzip(const std::string& path, const std::string& mode);
    ~Gzip();
    
    auto is_open(void) const -> bool override;
    auto good(void) const -> bool override;
    auto flush(void) -> bool override;
    auto read(std::uint8_t* data, std::uint32_t len) -> bool override;
    auto write(const std::uint8_t* data, std::uint32_t len) -> bool override;
    auto seek(std::uint32_t len, std::uint32_t ed) -> bool override;
    auto tell(void) -> std::uint32_t override;

private:
    gzFile file{nullptr};
    std::uint32_t getFileSize(void) override;
};

} // namespace mgb::io

#endif // #ifndef MGB_NO_GZIP
