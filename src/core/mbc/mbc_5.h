#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

static void GB_mbc5_write(struct GB_Core* gb, GB_U16 addr, GB_U8 value) { 
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
            const GB_U16 bank = (gb->cart.rom_bank & 0xFF00) | value;
            gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
        } break;

    // ROM BANK HIGH
        case 0x3: {
            // sets the 8th bit
            const GB_U16 bank = (gb->cart.rom_bank & 0x00FF) | ((value & 0x1) << 8);
            gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
        } break;

    // RAM BANK
        case 0x4: case 0x5:
            if (gb->cart.flags & MBC_FLAGS_RAM) {
                const GB_U8 bank = value & 0x0F;
                gb->cart.ram_bank = bank % gb->cart.ram_bank_max;
                GB_update_ram_banks(gb);
            }
            break;
            
        case 0xA: case 0xB:
            if (gb->cart.flags & MBC_FLAGS_RAM && gb->cart.ram_enabled) {
                gb->cart.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.ram_bank)] = value;
            }
            break;
    }
}

static const GB_U8* GB_mbc5_get_rom_bank(struct GB_Core* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

// todo: rtc support
static const GB_U8* GB_mbc5_get_ram_bank(struct GB_Core* gb, GB_U8 bank) {
	GB_UNUSED(bank);
    
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return MBC_NO_RAM;
	}
	return gb->cart.ram + (0x2000 * gb->cart.ram_bank);
}

#ifdef __cplusplus
}
#endif
