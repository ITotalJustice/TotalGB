#include "frontend/util/io/romloader.hpp"

#ifndef MGB_NO_7ZIP
#include "frontend/util/io/ifile_7z.hpp"
#endif
#ifndef MGB_NO_CFILE
#include "frontend/util/io/ifile_cfile.hpp"
#endif
#ifndef MGB_NO_GZIP
#include "frontend/util/io/ifile_gzip.hpp"
#endif
#include "frontend/util/io/ifile_mem.hpp"
#ifndef MGB_NO_RAR
#include "frontend/util/io/ifile_rar.hpp"
#endif
#ifndef MGB_NO_ZIP
#include "frontend/util/io/ifile_zip.hpp"
#endif
#ifndef MGB_NO_ZSTD
#include "frontend/util/io/ifile_zstd.hpp"
#endif

#include "frontend/util/util.hpp"
#include "frontend/util/mem.hpp"

#include <cstring>
#include <array>


namespace mgb::io {


RomLoader::RomLoader(const std::string& path) {
    const auto type = util::getExtType(path);

#ifndef MGB_NO_ZIP
    if (type == util::ExtType::ZIP) {
        Zip zip(path.c_str(), "rb");
        if (zip.is_open()) {
            this->file = std::make_unique<MemFile>(zip.readAll());   
        }
    }
#endif
    
#ifndef MGB_NO_GZIP
    if (type == util::ExtType::GZIP) {
        this->file = std::make_unique<Gzip>(path.c_str(), "rb");
    }
#endif

    if (type == util::ExtType::ROM) {
        this->file = std::make_unique<Cfile>(path.c_str(), "rb");
    }
}

RomLoader::RomLoader(const std::string& path, const std::uint8_t* data, std::size_t size) {
    const auto type = util::getExtType(path);

#ifndef MGB_NO_ZIP
    if (type == util::ExtType::ZIP) {
        Zip zip(data, size);
        if (zip.is_open()) {
            this->file = std::make_unique<MemFile>(zip.readAll());   
        }
    }
#endif

    if (type == util::ExtType::ROM) {
        std::vector<std::uint8_t> buf(size);
        std::memcpy(buf.data(), data, size);
        this->file = std::make_unique<MemFile>(std::move(buf));  
    }
}

bool RomLoader::read(std::uint8_t* data, std::uint32_t len) {
    return this->is_open() && this->file->read(data, len);
}

bool RomLoader::write(const std::uint8_t*, std::uint32_t) {
    return false;
}

bool RomLoader::seek(std::uint32_t off, std::uint32_t id) {
    return this->is_open() && this->file->seek(off, id);
}

bool RomLoader::flush(void) {
    return this->file->flush();
}

std::uint32_t RomLoader::tell(void) {
    return this->file->tell();
}

bool RomLoader::good(void) const {
    return this->file->good();
}

bool RomLoader::is_open(void) const {
    return this->file ? this->file->is_open() : false;
}

std::uint32_t RomLoader::getFileSize(void) {
    return this->file->size();
}

} // namespace mgb::io
