#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace mgb::mem {

enum class Mode {
    INFLATE, DEFLATE
};

enum class CompressionTypes {
    ZLIB, LZ4, ZSTD, LZMA
};

using Data = std::vector<std::uint8_t>;

// simple wrappers for compression libs (and my own delta flipflop).
// dst MUST be the same size OR larger than src.
// if successful, dst will autimatically shrunk to the new size.
// for example: dst size == 100. compressed size == 10. dst resized to 10.
auto Zlib(Data& dst, Data& src, Mode mode, int level = -1, bool gzip = true) -> bool;
auto Lz4(Data& dst, const Data& src, Mode mode, int level = 1) -> bool;
auto Zstd(Data& dst, const Data& src, Mode mode, int level = 1) -> bool;
auto Lzma(Data& dst, const Data& src, Mode mode, int level = 1, unsigned dict_size = 1 << 24, int lc = 3, int lp = 0, int pb = 2, int fb = 32, int threads = 1) -> bool;
auto Delta(const Data& keyframe, Data& dst, Mode mode, CompressionTypes type) -> std::pair<bool,std::size_t> ;

// Extremely fast and simple *same-size* delta encoding.
namespace delta {

using Callback = std::function<bool(std::size_t idx, std::uint8_t delta, std::uint8_t origional)>;

template<typename T>
constexpr void flipflop(const T& keyframe, T& delta, const std::size_t keyframe_size) noexcept {
    for (std::size_t i = 0; i < keyframe_size; i++) {
        delta[i] ^= keyframe[i];
    }
}
template<typename T>
constexpr void flipflop(const T& keyframe, T& delta, const std::size_t keyframe_size, std::size_t& delta_size) noexcept {
    delta_size = 0;
    for (std::size_t i = 0; i < keyframe_size; i++) {
        delta_size += (!!(delta[i] ^= keyframe[i]));
    }
}
template<typename T>
constexpr void flipflop(const T& keyframe, const T& delta, const std::size_t keyframe_size, Callback cb) noexcept {
    for (std::size_t i = 0; i < keyframe_size; i++) {
        if ((delta[i] ^ keyframe[i])) {
            if (!cb(i, delta[i], keyframe[i])) {
                return;
            }
        }
    }
}
template<typename T>
constexpr void flipflop(const T* keyframe, T* delta, const std::size_t keyframe_size) noexcept {
    for (std::size_t i = 0; i < keyframe_size; i++) {
        delta[i] ^= keyframe[i];
    }
}
template<typename T>
constexpr void flipflop(const T* keyframe, T* delta, const std::size_t keyframe_size, std::size_t* delta_size) noexcept {
    *delta_size = 0;
    for (std::size_t i = 0; i < keyframe_size; i++) {
        *delta_size += (!!(delta[i] ^= keyframe[i]));
    }
}
template<typename T>
constexpr void flipflop(const T* keyframe, const T* delta, const std::size_t keyframe_size, Callback cb) noexcept {
    for (std::size_t i = 0; i < keyframe_size; i++) {
        if ((delta[i] ^ keyframe[i])) {
            if (!cb(i, delta[i], keyframe[i])) {
                return;
            }
        }
    }
}

} // namespace delta
} // namespace gb::mem
