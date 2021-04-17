#include "core/mbc/mbc.h"
#include "core/internal.h"


void GB_mbc2_write(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	switch ((addr >> 12) & 0xF) {
	// RAM ENABLE / ROM BANK
        case 0x0: case 0x1: case 0x2: case 0x3:
            // if the 8th bit is not set, the value
            // controls the ram enable
            if ((addr & 0x100) == 0) {
                gb->cart.ram_enabled = (value & 0x0F) == 0x0A;
                GB_update_ram_banks(gb);
            } else {
                const uint8_t bank = (value & 0x0F) | ((value & 0x0F) == 0);
                gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
                GB_update_rom_banks(gb);
            }
            break;

    // RAM WRITE
        case 0xA: case 0xB: {
            if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
                return;
            }

            const uint8_t masked_value = (value & 0x0F) | 0xF0;
            const uint16_t masked_addr = addr & 0x1FF;

            gb->cart.ram[masked_addr] = masked_value;
        } break;
	}
}

struct MBC_RomBankInfo GB_mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
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

struct MBC_RamBankInfo GB_mbc2_get_ram_bank(struct GB_Core* gb) {
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return mbc_setup_empty_ram();
	}

	struct MBC_RamBankInfo info = {0};
    
    const uint8_t* ptr = gb->cart.ram + (0x2000 * gb->cart.ram_bank);

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i) {
        info.entries[i].ptr = ptr;
        info.entries[i].mask = 0x01FF;
    }

    return info;
}
