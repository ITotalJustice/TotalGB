#pragma once

#include "frontend/util/io/ifile_base.hpp"

#include <string>
#include <memory>


namespace mgb::io {


class RomLoader final : public IFile {
public:
    explicit RomLoader(const char* path);
    explicit RomLoader(const std::string& path);
    ~RomLoader() = default;

    auto is_open(void) const -> bool override;
    auto good(void) const -> bool override;
    auto flush(void) -> bool override;
    auto read(std::uint8_t* data, std::uint32_t len) -> bool override;
    auto write(const std::uint8_t* data, std::uint32_t len) -> bool override;
    auto seek(std::uint32_t len, std::uint32_t ed) -> bool override;
    auto tell(void) -> std::uint32_t override;

private:
    std::unique_ptr<IFile> file{nullptr};
    auto getFileSize(void) -> std::uint32_t override;
};

} // namespace mgb::io
