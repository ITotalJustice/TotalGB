#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#include <stdio.h>

/*
 08h  RTC S   Seconds   0-59 (0-3Bh)
 09h  RTC M   Minutes   0-59 (0-3Bh)
 0Ah  RTC H   Hours     0-23 (0-17h)
 0Bh  RTC DL  Lower 8 bits of Day Counter (0-FFh)
 0Ch  RTC DH  Upper 1 bit of Day Counter, Carry Bit, Halt Flag
       Bit 0  Most significant bit of Day Counter (Bit 8)
       Bit 6  Halt (0=Active, 1=Stop Timer)
       Bit 7  Day Counter Carry Bit (1=Counter Overflow)
*/

// this should be called at the end of each frame.
// this will first check if theres a RTC for the mbc
// if yes, then it will tick the counter
// the counter ticks from 0-59, resets at 60.
// at 60, it will finally tick the actual RTC.
// if the RTC is disabled, then the RTC is not ticked...

void GB_rtc_tick_frame(struct GB_Data* gb) {
    // check if we even have rtc?
    if ((gb->cart.flags & MBC_FLAGS_RTC) == 0) {
        return;
    }

    // as this func is called one a frame at roughly 60fps
    // we tick to 59 first then reset
    ++gb->cart.internal_rtc_counter;
    if (gb->cart.internal_rtc_counter < 0x3D) {
        return;
    }
    gb->cart.internal_rtc_counter = 0;

    enum GB_RtcFlags {
        OVERFLOW = 0x01,
        HALT = 0x40
    };

    if ((gb->cart.rtc.DH & HALT) == HALT) {
        return;
    }

    // this is a bit sphagetii, but basically tick the lowest entry
    // if overflow, reset and tick the next one, then repeat...
    ++gb->cart.rtc.S;
    if (gb->cart.rtc.S > 0x3B) {
        gb->cart.rtc.S = 0;
        
        ++gb->cart.rtc.M;
        if (gb->cart.rtc.M > 0x3B) {
            gb->cart.rtc.M = 0;

            ++gb->cart.rtc.H;
            if (gb->cart.rtc.H > 0x17) {
                gb->cart.rtc.H = 0;

                ++gb->cart.rtc.DL;
                if (gb->cart.rtc.DL == 0) { // wrap around to 0
                    gb->cart.rtc.DL = 0;

                    // if already set, we have overflowed the days counter!
                    // 0-511 days!
                    if ((gb->cart.rtc.DH & OVERFLOW) == OVERFLOW) {
                        // set the overflow bit but keep bit-6!
                        gb->cart.rtc.DH = (gb->cart.rtc.DH & 0x40) | 0x80;
                    } else {
                        ++gb->cart.rtc.DH;
                    }
                }
            }
        }
    }
}

// static const char* GB_mbc3_rtc_type(const struct GB_Data* gb) {
//     switch (gb->cart.rtc_mapped_reg) {
//         case GB_RTC_MAPPED_REG_S: return "GB_RTC_MAPPED_REG_S";
//         case GB_RTC_MAPPED_REG_M: return "GB_RTC_MAPPED_REG_M";
//         case GB_RTC_MAPPED_REG_H: return "GB_RTC_MAPPED_REG_H";
//         case GB_RTC_MAPPED_REG_DL: return "GB_RTC_MAPPED_REG_DL";
//         case GB_RTC_MAPPED_REG_DH: return "GB_RTC_MAPPED_REG_DH";
//     }

//     GB_UNREACHABLE(0xFF);
// }

static inline void GB_mbc3_rtc_write(struct GB_Data* gb, GB_U8 value) {
    switch (gb->cart.rtc_mapped_reg) {
        case GB_RTC_MAPPED_REG_S: gb->cart.rtc.S = value; break;
        case GB_RTC_MAPPED_REG_M: gb->cart.rtc.M = value; break;
        case GB_RTC_MAPPED_REG_H: gb->cart.rtc.H = value; break;
        case GB_RTC_MAPPED_REG_DL: gb->cart.rtc.DL = value; break;
        case GB_RTC_MAPPED_REG_DH: gb->cart.rtc.DH = value; break;
    }
}

#if GB_RTC_SPEEDHACK
static inline void GB_speed_hack_map_rtc_reg(struct GB_Data* gb) {
    switch (gb->cart.rtc_mapped_reg) {
        case GB_RTC_MAPPED_REG_S: gb->mmap[0xA] = &gb->cart.rtc.S; break;
        case GB_RTC_MAPPED_REG_M: gb->mmap[0xA] = &gb->cart.rtc.M; break;
        case GB_RTC_MAPPED_REG_H: gb->mmap[0xA] = &gb->cart.rtc.H; break;
        case GB_RTC_MAPPED_REG_DL: gb->mmap[0xA] = &gb->cart.rtc.DL; break;
        case GB_RTC_MAPPED_REG_DH: gb->mmap[0xA] = &gb->cart.rtc.DH; break;
    }
}
#endif // GB_RTC_SPEEDHACK

static void GB_mbc3_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) { 
    switch ((addr >> 12) & 0xF) {
	// RAM / RTC REGISTER ENABLE
        case 0x0: case 0x1:
            gb->cart.ram_enabled = (value & 0x0F) == 0x0A;
            GB_update_ram_banks(gb);
            break;
            
    // ROM BANK
        case 0x2: case 0x3:
            gb->cart.rom_bank = ((value) | (value == 0)) % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
            break;

    // RAM BANK / RTC REGISTER
        case 0x4: case 0x5:
            // set bank 0-3
            if (value <= 0x03) {
                // NOTE: can this be out of range?
                gb->cart.ram_bank = value & 0x3;
                gb->cart.in_ram = GB_TRUE;
                GB_update_ram_banks(gb);
            }
            // if we have rtc and in range, set the mapped rtc reg
            else if (GB_has_mbc_flags(gb, MBC_FLAGS_RTC) && value >= 0x08 && value <= 0x0C) {
                gb->cart.rtc_mapped_reg = value - 0x08;
                gb->cart.in_ram = GB_FALSE;
            
            #if GB_RTC_SPEEDHACK
                GB_speed_hack_map_rtc_reg(gb);
            #endif // #if GB_RTC_SPEEDHACK
            }
            break;

    // LATCH CLOCK DATA
        case 0x6: case 0x7:
            break;

        case 0xA: case 0xB:
            if (GB_has_mbc_flags(gb, MBC_FLAGS_RAM) && gb->cart.ram_enabled) {
                if (gb->cart.in_ram) {
                    gb->cart.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.ram_bank)] = value;
                }
                else if (GB_has_mbc_flags(gb, MBC_FLAGS_RTC)) {
                    // printf("writing to rtc reg: %s\n", GB_mbc3_rtc_type(gb));
                    GB_mbc3_rtc_write(gb, value);
                }
            }
            break;
    }
}

static const GB_U8* GB_mbc3_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

// todo: rtc support
static const GB_U8* GB_mbc3_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	GB_UNUSED(bank);
    
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled || !gb->cart.in_ram) {
		return MBC_NO_RAM;
	}

	return gb->cart.ram + (0x2000 * gb->cart.ram_bank);
}

#ifdef __cplusplus
}
#endif
