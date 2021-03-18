#pragma once

#include <string>

namespace mgb::util {

enum class ExtType { // valid extensions
    UNK, ROM, SAVE, RTC, STATE, ZIP, GZIP, LZMA, RAR,
    IPS, BPS, 
};

constexpr auto cexprHash(const char *str, std::size_t v = 0) noexcept -> std::size_t {
    return (*str == 0) ? v : 31 * cexprHash(str + 1) + *str;
}

constexpr auto cexprStrlen(const char *str) noexcept -> std::size_t {
    return (*str != 0) ? 1 + cexprStrlen(str + 1) : 0;
}

auto appendExt(const std::string& str, const std::string& ext) -> std::string;
auto getSavePathFromString(const std::string& path) -> std::string;
auto getStatePathFromString(const std::string& path) -> std::string;
auto getRtcPathfromString(const std::string& path) -> std::string;

auto getExtPos(const std::string& str) -> std::size_t;
auto getExt(const std::string& str) -> std::string;
auto getExtType(const std::string& str) -> ExtType;
auto isExtType(const std::string& path, const ExtType wanted) -> bool;

} // namespace mgb::util
