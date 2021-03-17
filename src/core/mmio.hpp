#pragma once

#include "common.hpp"

namespace dmg {

struct Mmio {
    std::array<const u8*, 0x10> mmap{};

    std::array<std::array<u8, 0x2000>, 2> vram{};
    std::array<u8, 0xA0> oam{};

    std::array<std::array<u8, 4>, 8> bg_colour{};
    std::array<std::array<u8, 4>, 8> obj_colour{};

    std::array<u8, 0x40> bg_palette{};
    std::array<u8, 0x40> obj_palette{};

    std::array<std::array<u8, 0x1000>, 8> wram{};
    std::array<u8, 0x80> hram{};

    u8 IE;
    u8 IF;

    // these are tests for if they work in constexpr (they do!)
    constexpr auto setup_mmap() noexcept -> void {
        this->mmap[0] = this->vram[0].data();
    }

    constexpr auto io_read(const u16 addr) noexcept -> u8 {
        switch (addr & 0x7F) {
            case 0x00: // joypad
                return 0xFF;
            case 0x01: // serial
                return 0xFF;
            case 0x02: // serial
                return 0xFF;
            case 0x4D: // double speed
                return 0xFF;
            case 0x70: // SVBK
                return 0xFF;
        }

        return 0xFF;
    }

    constexpr auto io_write(const u16 addr, [[maybe_unused]] const u8 v) noexcept -> void {
        switch (addr & 0x7F) {
            case 0x00: // joypad
                break;
            case 0x01: // serial
                break;
            case 0x02: // serial
                break;
            case 0x4D: // double speed
                break;
            case 0x70: // SVBK
                break;
        }
    }

    constexpr auto read(const u16 addr) -> u8 {
        return 0;
        if (addr < 0xFE00) [[likely]] {
            return this->mmap[(addr >> 12)][addr & 0x0FFF];
        } else if (addr <= 0xFE9F) {
            return this->oam[addr & 0x9F];
        } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
            return this->io_read(addr);
        } else if (addr >= 0xFF80) {
            return this->hram[addr & 0x7F];
        } else if (addr == 0xFFFF) {
            return 0xFF;
        }

        return 0xFF;
    }

    constexpr auto write(const u16 addr, const u8 v) -> void {
        if (addr < 0xFE00) [[likely]] {
            switch ((addr >> 12) & 0xF) {
                case 0x0: case 0x1: case 0x2: case 0x3: case 0x4:
                case 0x5: case 0x6: case 0x7: case 0xA: case 0xB:
                    // mbc write
                    break;
                case 0x8: case 0x9:
                    this->vram[0][addr & 0x1FFF] = v;
                    break;
                case 0xC: case 0xE:
                    this->wram[0][addr & 0x0FFF] = v;
                    break;
                case 0xD: case 0xF:
                    this->wram[1][addr & 0x0FFF] = v;
                    break;
            }
        } else if (addr <= 0xFE9F) {
            this->oam[addr & 0x9F] = v;
        } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
            this->io_write(addr, v);
        } else if (addr >= 0xFF80) {
            this->hram[addr & 0x7F] = v;
        } else if (addr == 0xFFFF) {
            // IE
        }
    }
};

} // namespace dmg
