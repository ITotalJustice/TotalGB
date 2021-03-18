#include "mem.hpp"

#ifndef MGB_NO_GZIP
#include <zlib/zlib.h>
#endif
#ifndef MGB_NO_LZ4
#include <lz4/lz4.h>
#endif
#ifndef MGB_NO_ZSTD
#include <zstd/zstd.h>
#endif
#ifndef MGB_NO_7ZIP
#include <lzma/LzmaLib.h>
#endif

namespace mgb::mem {

#ifndef MGB_NO_GZIP
bool Zlib(Data& dst, Data& src, Mode mode, int level, bool gzip) {
    z_stream stream{};
    stream.avail_in = src.size();
    stream.next_in = src.data();
    stream.avail_out = dst.size();
    stream.next_out = dst.data();

    const int window_bits = gzip ? 16 + MAX_WBITS : MAX_WBITS;

    if (mode == Mode::DEFLATE) {
        auto result = deflateInit2(&stream, level, Z_DEFLATED, window_bits, 9, Z_DEFAULT_STRATEGY);
        if (result != Z_OK) {
            return false;
        }
        result = deflate(&stream, Z_FINISH);
        deflateEnd(&stream);
        if (result != Z_STREAM_END) {
            return false;
        }
        dst.resize(stream.total_out);
    } else {
        auto result = inflateInit2(&stream, window_bits);
        result = inflate(&stream, Z_FINISH);
        result = inflateEnd(&stream);
        if (result != Z_OK || stream.total_out < dst.size()) {
            return false;
        }
    }

    return true;
}
#endif // #ifndef MGB_NO_GZIP

#ifndef MGB_NO_LZ4
bool Lz4(Data& dst, const Data& src, Mode mode, int level) {
    if (mode == Mode::DEFLATE) {
        const auto result = LZ4_compress_fast(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()), src.size(), dst.size(), level);
        if (result == 0) {
            return false;
        }
        dst.resize(result);
    } else {
        const auto result = LZ4_decompress_safe(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()), src.size(), dst.size());
        if (result == 0) {
            return false;
        }
    }

    return true;
}
#endif // #ifndef MGB_NO_LZ4

#ifndef MGB_NO_ZSTD
bool Zstd(Data& dst, const Data& src, Mode mode, int level) {
    if (mode == Mode::DEFLATE) {
        const auto result = ZSTD_compress(static_cast<void*>(dst.data()), dst.size(), static_cast<const void*>(src.data()), src.size(), level);
        if (result == 0) {
            return false;
        }
        dst.resize(result);
    } else {
        const auto result = ZSTD_decompress(static_cast<void*>(dst.data()), dst.size(), static_cast<const void*>(src.data()), src.size());
        if (result == 0) {
            return false;
        }
    }

    return true;
}
#endif // #ifndef MGB_NO_ZSTD

#ifndef MGB_NO_7ZIP
bool Lzma(Data& dst, const Data& src, Mode mode, int level, unsigned dict_size, int lc, int lp, int pb, int fb, int threads) {
    if (mode == Mode::DEFLATE) {
        auto dst_size = dst.size();
        std::uint8_t props[5];
        std::size_t prop_size = 5;
        const auto result = LzmaCompress(dst.data(), &dst_size, src.data(), src.size(),
            props, &prop_size, level, dict_size, lc, lp, pb, fb, threads);
        if (result != SZ_OK) {
            return false;
        }
        dst.resize(dst_size);
    } else {
        auto dst_size = dst.size();
        auto src_size = src.size();
        const std::uint8_t props[5]{0};
        const std::size_t prop_size = 5;
        const auto result = LzmaUncompress(dst.data(), &dst_size, src.data(), &src_size, props, prop_size);
        if (result != SZ_OK) {
            return false;
        }
    }

    return true;
}
#endif // #ifndef MGB_NO_7ZIP

auto Delta(const Data& keyframe, Data& delta, Mode mode, CompressionTypes type) -> std::pair<bool,std::size_t>  {
    Data dst(keyframe.size());
    std::size_t delta_size = 0;

    const auto func = [&]() {
        switch (type) {
        #ifndef MGB_NO_GZIP
            case CompressionTypes::ZLIB:
            Zlib(dst, delta, mode);
            break;
        #endif // #ifndef MGB_NO_GZIP
        #ifndef MGB_NO_LZ4
            case CompressionTypes::LZ4:
            Lz4(dst, delta, mode);
            break;
        #endif // #ifndef MGB_NO_LZ4
        #ifndef MGB_NO_ZSTD
            case CompressionTypes::ZSTD:
            Zstd(dst, delta, mode);
            break;
        #endif // #ifndef MGB_NO_ZSTD
        #ifndef MGB_NO_7ZIP
            case CompressionTypes::LZMA:
            Lzma(dst, delta, mode);
            break;
        #endif // #ifndef MGB_NO_7ZIP
            default:
            return false;
        }

        return true;
    };

    if (mode == Mode::DEFLATE) {
        delta::flipflop(keyframe, delta, keyframe.size(), delta_size);
        if (!func()) {
            return std::make_pair(false, 0);
        }
    } else {
        if (!func()) {
            return std::make_pair(false, 0);
        }
        delta::flipflop(keyframe, dst, keyframe.size());
    }

    delta = std::move(dst);
    return std::make_pair(true, delta_size);
}

} // namespace mgb::mem
