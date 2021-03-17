#pragma once

#include "common.hpp"

#include <cstring> // memcpy
#include <string_view>
#include <optional>
#include <span>

namespace dmg {

constexpr auto SCREEN_WIDTH = 160;
constexpr auto SCREEN_HEIGHT = 144;

struct CartHeader {
	u8 entry_point[0x4];
	u8 logo[0x30];
	char title[0x10];
	u16 new_licensee_code;
	u8 sgb_flag;
	u8 cart_type;
	u8 rom_size;
	u8 ram_size;
	u8 destination_code;
	u8 old_licensee_code;
	u8 rom_version;
	u8 header_checksum;
	u16 global_checksum;

    constexpr auto get_title() -> std::string_view {
        return std::string_view{this->title, std::size(this->title)};
    }
};

[[nodiscard]]
auto get_cart_header(std::span<const u8> data) -> std::optional<CartHeader> {
    if (data.size() < sizeof(CartHeader)) {
        return {};
    }

    CartHeader header{};
    std::memcpy(&header, data.data(), sizeof(header));

    return header;
}

} // namespace dmg
