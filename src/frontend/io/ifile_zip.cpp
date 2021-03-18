#ifndef MGB_NO_ZIP

#include "ifile_zip.hpp"
#include "../util/util.hpp"

namespace mgb::io {

Zip::Zip(const char* path, const char* mode) {
    this->file = unzOpen(path);
    if (!this->file) {
        return;
    }

    auto result = unzGoToFirstFile(this->file);

    // find the gb / gbc file, else close
    while (result == UNZ_OK) {
        char file_name[1024]{0};
        unz_file_info info;
        if (unzGetCurrentFileInfo(this->file, &info, file_name, sizeof(file_name),
            nullptr, 0, nullptr, 0) != UNZ_OK) {
            break;
        }

        if (util::isExtType(file_name, util::ExtType::ROM)) {
            if (unzOpenCurrentFile(this->file) == UNZ_OK) {
                this->file_info = info;
                return;
            }
        }

        result = unzGoToNextFile(this->file);
    }

    // we failed, so close the file early and null it
    unzClose(this->file);
    this->file = nullptr;
}

Zip::~Zip() {
    if (this->file) {
        unzCloseCurrentFile(this->file);
        unzClose(this->file);
    }
}

bool Zip::is_open(void) const {
    return (this->file);
}

bool Zip::good(void) const {
    return true;
}

bool Zip::flush(void) {
    return true;
}

bool Zip::read(std::uint8_t* data, std::uint32_t len) {
    return this->file && unzReadCurrentFile(this->file, data, len) > 0;
}

bool Zip::write(const std::uint8_t* data, std::uint32_t len) {
    return false;
}

bool Zip::seek(std::uint32_t len, std::uint32_t ed) {
    unzSeek(this->file, len, ed);
    return this->file && true;
}

std::uint32_t Zip::tell(void) {
    return this->file ? static_cast<std::uint32_t>(unzTell(this->file)) : 0;
}

std::vector<std::uint8_t> Zip::readAll(void) {
    std::vector<std::uint8_t> data(this->size());
    unzReadCurrentFile(this->file, static_cast<voidp>(data.data()), file_info.uncompressed_size);
    return data;
}

std::uint32_t Zip::getFileSize(void) {
    return this->file_size = file_info.uncompressed_size;
}

} // namespace io

#endif // #ifndef MGB_NO_ZIP
