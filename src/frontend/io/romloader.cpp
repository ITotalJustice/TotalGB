#include "romloader.hpp"

#ifndef MGB_NO_7ZIP
#include "ifile_7z.hpp"
#endif
#ifndef MGB_NO_CFILE
#include "ifile_cfile.hpp"
#endif
#ifndef MGB_NO_GZIP
#include "ifile_gzip.hpp"
#endif
#include "ifile_mem.hpp"
#ifndef MGB_NO_RAR
#include "ifile_rar.hpp"
#endif
#ifndef MGB_NO_ZIP
#include "ifile_zip.hpp"
#endif
#ifndef MGB_NO_ZSTD
#include "ifile_zstd.hpp"
#endif

#include "../util/util.hpp"

#include <cstring>
#include <array>

namespace mgb::io {

RomLoader::RomLoader(const char* path) {
    const auto type = util::getExtType(path);

    if (type == util::ExtType::ZIP) {
#ifndef MGB_NO_ZIP
        Zip zip(path, "rb");
        if (zip.is_open()) {
            this->file = std::make_unique<MemFile>(zip.readAll());   
        }
#endif
    } else if (type == util::ExtType::GZIP) {
#ifndef MGB_NO_GZIP
        this->file = std::make_unique<Gzip>(path, "rb");
#endif
    } else if (type == util::ExtType::ROM) {
        this->file = std::make_unique<Cfile>(path, "rb");
    }
}

RomLoader::RomLoader(const std::string& path) : RomLoader(path.c_str()) { }

bool RomLoader::read(std::uint8_t* data, std::uint32_t len) {
    return this->is_open() && this->file->read(data, len);
}

bool RomLoader::write(const std::uint8_t* data, std::uint32_t len) {
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
