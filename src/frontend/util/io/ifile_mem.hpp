#pragma once

#include "frontend/util/io/ifile_base.hpp"

#include <cstdint>
#include <vector>


namespace mgb::io {


class MemFile final : public IFile {
public:
    MemFile(std::vector<std::uint8_t>&& _data);
    
    auto is_open(void) const -> bool override;
    auto good(void) const -> bool override;
    auto flush(void) -> bool override;
    auto read(std::uint8_t* data, std::uint32_t len) -> bool override;
    auto write(const std::uint8_t* data, std::uint32_t len) -> bool override;
    auto seek(std::uint32_t len, std::uint32_t ed) -> bool override;
    auto tell(void) -> std::uint32_t override;

private:
    std::vector<std::uint8_t> data{};
    std::uint32_t pos{0};
    std::uint32_t getFileSize(void) override;
};

} // namespace mgb::io
