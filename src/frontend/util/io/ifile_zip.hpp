#pragma once

#ifndef MGB_NO_ZIP

#include "frontend/util/io/ifile_base.hpp"

#include <minizip/unzip.h>
#include <cstdint>
#include <vector>
#include <memory>

// don't want to bloat the public name space by including headers
// so make this opaque
extern "C" {
typedef struct ourmemory_s ourmemory_t;
}

namespace mgb::io {


class Zip final : public IFile {
public:
    Zip(const char* path, const char* mode);
    Zip(const uint8_t* buf, std::size_t size);
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
    auto SetupFromFile() -> bool;
    std::uint32_t getFileSize(void) override;

private:
    std::unique_ptr<ourmemory_t> ourmemory; // only used for mem open
    unzFile file{nullptr};
    unz_file_info file_info{};
};

} // namespace mgb::io

#endif // #ifndef MGB_NO_ZIP
