#pragma once

#ifndef MGB_NO_ZIP

#include "ifile_base.hpp"

#include <minizip/unzip.h>
#include <cstdint>
#include <vector>

namespace mgb::io {

class Zip final : public IFile {
public:
    Zip(const char* path, const char* mode);
    ~Zip();
    
    auto is_open(void) const -> bool override;
    auto good(void) const -> bool override;
    auto flush(void) -> bool override;
    auto read(std::uint8_t* data, std::uint32_t len) -> bool override;
    auto write(const std::uint8_t* data, std::uint32_t len) -> bool override;
    auto seek(std::uint32_t len, std::uint32_t ed) -> bool override;
    auto tell(void) -> std::uint32_t override;

    auto readAll(void) -> std::vector<std::uint8_t>;

private:
    unzFile file{nullptr};
    unz_file_info file_info{};
    std::uint32_t getFileSize(void) override;
};

} // namespace mgb::io

#endif // #ifndef MGB_NO_ZIP
