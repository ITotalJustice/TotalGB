#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

static void GB_mbc1_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	switch ((addr >> 12) & 0xF) {
	// RAM ENABLE
        case 0x0: case 0x1:
            gb->cart.ram_enabled = (!!(value & 0xA));
            GB_update_ram_banks(gb);
            break;
            
    // ROM BANK
        case 0x2: case 0x3: // ((value & 0x1F) + !(value & 0x1))
            gb->cart.rom_bank = value & 0x1F;
            if (!gb->cart.rom_bank) gb->cart.rom_bank = 1;
            GB_update_rom_banks(gb);
            break;

    // ROM / RAM BANK
        case 0x4: case 0x5:
            if (gb->cart.bank_mode) {
                gb->cart.ram_bank = value & 3;
                GB_update_ram_banks(gb);
            } else {
                gb->cart.rom_bank = (gb->cart.rom_bank & 0x1F) | ((value & 224));
                GB_update_rom_banks(gb);
            }
            break;

    // ROM / RAM MODE
        case 0x6: case 0x7:
            gb->cart.bank_mode = value & 1;
            break;

    // RAM BANK X
        case 0xA: case 0xB:
            if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
                return;
            }
            gb->cart.ram[(addr & 0x1FFF) + (0x2000 * (gb->cart.bank_mode ? gb->cart.ram_bank : 0))] = value;
            break;
	}
}

static const GB_U8* GB_mbc1_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

static const GB_U8* GB_mbc1_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(bank);
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return MBC_NO_RAM;
	}

	if (gb->cart.bank_mode) {
		return gb->cart.ram + (0x2000 * gb->cart.ram_bank);
	}
    
	return gb->cart.ram;
}

#ifdef __cplusplus
}
#endif
