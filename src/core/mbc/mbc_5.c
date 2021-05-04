#include "core/mbc/mbc.h"
#include "core/internal.h"

#include <assert.h>


void GB_mbc5_write(struct GB_Core* gb, uint16_t addr, uint8_t value) {
    switch ((addr >> 12) & 0xF) {
    // RAM
        case 0x0: case 0x1:
            // gbctr states that only 0xA enables ram
            // every other value disables it...
            gb->cart.ram_enabled = value == 0x0A;
            GB_update_ram_banks(gb);
            break;

    // ROM BANK LOW
        case 0x2: {
            // sets bits 0-7
            const uint16_t bank = (gb->cart.rom_bank & 0xFF00) | value;
            gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
        } break;

    // ROM BANK HIGH
        case 0x3: {
            // sets the 8th bit
            const uint16_t bank = (gb->cart.rom_bank & 0x00FF) | ((value & 0x1) << 8);
            gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
        } break;

    // RAM BANK
        case 0x4: case 0x5:
            if (gb->cart.flags & MBC_FLAGS_RAM) {
                const uint8_t bank = value & 0x0F;
                gb->cart.ram_bank = bank % gb->cart.ram_bank_max;
                GB_update_ram_banks(gb);
            }
            break;

        case 0xA: case 0xB:
            if ((gb->cart.flags & MBC_FLAGS_RAM) && gb->cart.ram_enabled) {
                gb->cart.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.ram_bank)] = value;
            }
            break;
    }
}

struct MBC_RomBankInfo GB_mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
    struct MBC_RomBankInfo info = {0};
    const uint8_t* ptr = NULL;

    if (bank == 0) {
        ptr = gb->cart.rom;
    }
    else {
        ptr = gb->cart.rom + (gb->cart.rom_bank * 0x4000);
    }

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i) {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}

struct MBC_RamBankInfo GB_mbc5_get_ram_bank(struct GB_Core* gb) {
    if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
        return mbc_setup_empty_ram();
    }

    struct MBC_RamBankInfo info = {0};

    const uint8_t* ptr = gb->cart.ram + (0x2000 * gb->cart.ram_bank);

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i) {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}
