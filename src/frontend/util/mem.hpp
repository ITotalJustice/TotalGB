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
#ifndef MGB_NO_ZLIB
    ZLIB,
#endif
#ifndef MGB_NO_LZ4
    LZ4,
#endif
#ifndef MGB_NO_ZSTD
    ZSTD,
#endif
#ifndef MGB_NO_LZMA
    LZMA
#endif
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

} // namespace gb::mem
