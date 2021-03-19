#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

static void GB_mbc5_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) { 
    switch ((addr >> 12) & 0xF) {
    // RAM / RTC REGISTER ENABLE
        case 0x0: case 0x1:
            gb->cart.ram_enabled = (!!(value & 0xA));
            GB_update_ram_banks(gb);
            break;
        
    // ROM BANK LOW
        case 0x2:
            gb->cart.rom_bank = (gb->cart.rom_bank & 0xFF00) | value;
            GB_update_rom_banks(gb);
            break;

    // ROM BANK HIGH
        case 0x3:
            gb->cart.rom_bank = (gb->cart.rom_bank & 0x00FF) | (value << 8);
            GB_update_rom_banks(gb);
            break;

    // RAM BANK / RTC REGISTER
        case 0x4: case 0x5:
            gb->cart.ram_bank = value & 0xF;
            GB_update_ram_banks(gb);
            break;

    // LATCH CLOCK DATA
        case 0x6: case 0x7:
            break;
            
        case 0xA: case 0xB:
            if (gb->cart.ram_enabled) {
                gb->cart.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.ram_bank)] = value;
            }
            break;
    }
}

static const GB_U8* GB_mbc5_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

// todo: rtc support
static const GB_U8* GB_mbc5_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(bank);
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return MBC_NO_RAM;
	}

	return gb->cart.ram + (0x2000 * gb->cart.ram_bank);
}

#ifdef __cplusplus
}
#endif