#pragma once

#include "frontend/util/io/ifile_base.hpp"


#include <cstdint>
#include <cstdio>
#include <string>


namespace mgb::io {


class Cfile final : public IFile {
public:
    Cfile(const char* path, const char* mode);
    Cfile(const std::string& path, const std::string& mode);
    ~Cfile();
    
    auto is_open(void) const -> bool override;
    auto good(void) const -> bool override;
    auto flush(void) -> bool override;
    auto read(std::uint8_t* data, std::uint32_t len) -> bool override;
    auto write(const std::uint8_t* data, std::uint32_t len) -> bool override;
    auto seek(std::uint32_t len, std::uint32_t ed) -> bool override;
    auto tell(void) -> std::uint32_t override;

private:
    FILE* file{nullptr};
    auto getFileSize(void) -> std::uint32_t override;
};

} // namespace io
