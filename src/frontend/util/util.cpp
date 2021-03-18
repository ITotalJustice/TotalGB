#include "util.hpp"

namespace mgb {

auto util::appendExt(const std::string& str, const std::string& ext) -> std::string {
    const auto pos = util::getExtPos(str);
    if (pos == str.npos) {
        return str + ext;
    }
    return (str.substr(0, pos) + ext);
}

auto util::getSavePathFromString(const std::string& path) -> std::string {
    return util::appendExt(path, ".sav");
}

auto util::getStatePathFromString(const std::string& path) -> std::string {
    return util::appendExt(path, ".state");
}

auto util::getRtcPathfromString(const std::string& path) -> std::string {
    return util::appendExt(path, ".rtc");
}

auto util::getExtPos(const std::string& str) -> std::size_t {
    return str.find_last_of('.');
}

auto util::getExt(const std::string& str) -> std::string {
    const auto pos = str.find_last_of('.');
    if (pos == str.npos) {
        return nullptr;
    }

    return str.substr(pos);
}

auto util::getExtType(const std::string& str) -> util::ExtType {
    const auto ext = getExt(str);
    if (ext.empty()) {
        return util::ExtType::UNK;
    }

    switch (util::cexprHash(ext.c_str())) {
        case util::cexprHash(".gb"): case util::cexprHash(".GB"):
        case util::cexprHash(".gbc"): case util::cexprHash(".GBC"):
        case util::cexprHash(".bin"): case util::cexprHash(".BIN"):
        case util::cexprHash(".sgb"): case util::cexprHash(".SGB"):
        case util::cexprHash(".cgb"): case util::cexprHash(".CGB"):
            return util::ExtType::ROM;

        case util::cexprHash(".sav"): case util::cexprHash(".SAV"):
            return util::ExtType::SAVE;

        case util::cexprHash(".rtc"): case util::cexprHash(".RTC"):
            return util::ExtType::RTC;

        case util::cexprHash(".state"): case util::cexprHash(".STATE"):
            return util::ExtType::STATE;

        case util::cexprHash(".zip"): case util::cexprHash(".ZIP"):
            return util::ExtType::ZIP;

        case util::cexprHash(".gzip"): case util::cexprHash(".GZIP"):
        case util::cexprHash(".gz"): case util::cexprHash(".GZ"):
            return util::ExtType::GZIP;

        case util::cexprHash(".7z"): case util::cexprHash(".7Z"):
            return util::ExtType::LZMA;

        case util::cexprHash(".rar"): case util::cexprHash(".RAR"):
            return util::ExtType::RAR;

        case util::cexprHash(".ips"): case util::cexprHash(".IPS"):
            return util::ExtType::IPS;

        case util::cexprHash(".bps"): case util::cexprHash(".BPS"):
        case util::cexprHash(".bpm"): case util::cexprHash(".BPM"):
            return util::ExtType::BPS;
    }

    return util::ExtType::UNK;
}

auto util::isExtType(const std::string& path, const ExtType wanted) -> bool {
    return wanted == util::getExtType(path);
}

} // namespace mgb
