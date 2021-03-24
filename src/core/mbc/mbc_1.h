#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#include <stdio.h>

static void GB_mbc1_write(struct GB_Core* gb, GB_U16 addr, GB_U8 value) {
	switch ((addr >> 12) & 0xF) {
	// RAM ENABLE
        case 0x0: case 0x1:
            // check the lower 4-bits for any value with 0xA
            gb->cart.ram_enabled = (value & 0xF) == 0xA;
            GB_update_ram_banks(gb);
            break;
            
    // ROM BANK
        case 0x2: case 0x3: {
            // check only 5-bits, cannot be 0
            gb->cart.rom_bank_lo = (value & 0x1F) | ((value & 0x1F) == 0);

            // TODO: idk what to do here...
            // if (gb->cart.bank_mode) {
                gb->cart.rom_bank = ((gb->cart.rom_bank_hi << 5) | (gb->cart.rom_bank_lo)) % gb->cart.rom_bank_max;
            // } else {
                // gb->cart.rom_bank = gb->cart.rom_bank_lo % gb->cart.rom_bank_max;
            // }
            
            GB_update_rom_banks(gb);
        } break;

    // ROM / RAM BANK
        case 0x4: case 0x5:
            if (gb->cart.rom_bank_max > 18) {
                gb->cart.rom_bank_hi = value & 0x3;
                gb->cart.rom_bank = ((gb->cart.rom_bank_hi << 5) | gb->cart.rom_bank_lo) % gb->cart.rom_bank_max;
                GB_update_rom_banks(gb);
            }

            if (gb->cart.bank_mode) {
                if ((gb->cart.flags & MBC_FLAGS_RAM)) {
                    gb->cart.ram_bank = (value & 0x3) % gb->cart.ram_bank_max;
                    GB_update_ram_banks(gb);
                }
            }
            break;

    // ROM / RAM MODE
        case 0x6: case 0x7:
            gb->cart.bank_mode = value & 0x1;
            
            if (gb->cart.rom_bank_max > 18) {
                if (gb->cart.bank_mode == 0) {
                    gb->cart.rom_bank = gb->cart.rom_bank_lo;
                }
            }
            GB_update_rom_banks(gb);
            break;

    // RAM BANK X
        case 0xA: case 0xB:
            if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
                return;
            }
            gb->cart.ram[(addr & 0x1FFF) + (0x2000 * (gb->cart.bank_mode == 1 ? gb->cart.ram_bank : 0))] = value;
            break;
	}
}

static const GB_U8* GB_mbc1_get_rom_bank(struct GB_Core* gb, GB_U8 bank) {
	if (bank == 0) {
        if (gb->cart.rom_bank_max > 18 && gb->cart.bank_mode == 1) {
            GB_U16 b = (gb->cart.rom_bank_hi << 5);
            if (b > gb->cart.rom_bank_max) {
                b %= gb->cart.rom_bank_max;
            }
            return gb->cart.rom + (b * 0x4000);
	    } else {
		    return gb->cart.rom;
        }
    }
	
    // in mode 0, always read from bank 0
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

static const GB_U8* GB_mbc1_get_ram_bank(struct GB_Core* gb, GB_U8 bank) {
	GB_UNUSED(bank);
    
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return MBC_NO_RAM;
	}

    // in mode 0, always read from bank 0
    return gb->cart.ram + (0x2000 * (gb->cart.bank_mode == 1 ? gb->cart.ram_bank : 0));
}    

#ifdef __cplusplus
}
#endif
