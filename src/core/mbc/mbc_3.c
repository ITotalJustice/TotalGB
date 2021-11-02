#include "../internal.h"
#include "../gb.h" // for has flags functions
#include "mbc.h"

#include <assert.h>

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

void GB_rtc_tick_frame(struct GB_Core* gb)
{
    // check if we even have rtc?
    if ((gb->cart.flags & MBC_FLAGS_RTC) == 0)
    {
        return;
    }

    enum RtcFlags
    {
        RtcFlags_OVERFLOW = 0x01,
        RtcFlags_HALT = 0x40
    };

    if ((gb->cart.rtc.DH & RtcFlags_HALT) == RtcFlags_HALT)
    {
        return;
    }

    // this is a bit sphagetii, but basically tick the lowest entry
    // if overflow, reset and tick the next one, then repeat...
    ++gb->cart.rtc.S;
    if (gb->cart.rtc.S > 59)
    {
        gb->cart.rtc.S = 0;

        ++gb->cart.rtc.M;
        if (gb->cart.rtc.M > 59)
        {
            gb->cart.rtc.M = 0;

            ++gb->cart.rtc.H;
            if (gb->cart.rtc.H > 23)
            {
                gb->cart.rtc.H = 0;

                ++gb->cart.rtc.DL;
                if (gb->cart.rtc.DL == 0)
                { // wrap around to 0
                    gb->cart.rtc.DL = 0;

                    // if already set, we have overflowed the days counter!
                    // 0-511 days!
                    if ((gb->cart.rtc.DH & RtcFlags_OVERFLOW) == RtcFlags_OVERFLOW)
                    {
                        // set the overflow bit but keep bit-6!
                        gb->cart.rtc.DH = (gb->cart.rtc.DH & 0x40) | 0x80;
                    }
                    else
                    {
                        ++gb->cart.rtc.DH;
                    }
                }
            }
        }
    }
}

static inline void mbc3_rtc_write(struct GB_Core* gb, uint8_t value)
{
    // TODO: cap the values so they aren't invalid!
    switch (gb->cart.rtc_mapped_reg)
    {
        case GB_RTC_MAPPED_REG_S: gb->cart.rtc.S = value; break;
        case GB_RTC_MAPPED_REG_M: gb->cart.rtc.M = value; break;
        case GB_RTC_MAPPED_REG_H: gb->cart.rtc.H = value; break;
        case GB_RTC_MAPPED_REG_DL: gb->cart.rtc.DL = value; break;
        case GB_RTC_MAPPED_REG_DH: gb->cart.rtc.DH = value; break;
    }
}

static inline void GB_speed_hack_map_rtc_reg(struct GB_Core* gb)
{
    uint8_t* ptr = NULL;

    switch (gb->cart.rtc_mapped_reg)
    {
        case GB_RTC_MAPPED_REG_S: ptr = &gb->cart.rtc.S; break;
        case GB_RTC_MAPPED_REG_M: ptr = &gb->cart.rtc.M; break;
        case GB_RTC_MAPPED_REG_H: ptr = &gb->cart.rtc.H; break;
        case GB_RTC_MAPPED_REG_DL: ptr = &gb->cart.rtc.DL; break;
        case GB_RTC_MAPPED_REG_DH: ptr = &gb->cart.rtc.DH; break;
    }

    gb->mmap[0xA].ptr = ptr;
    gb->mmap[0xA].mask = 0;
    gb->mmap[0xB].ptr = ptr;
    gb->mmap[0xB].mask = 0;
}

void mbc3_write(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    switch ((addr >> 12) & 0xF)
    {
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
            if (value <= 0x03)
            {
                // NOTE: can this be out of range?
                gb->cart.ram_bank = value & 0x3;
                gb->cart.in_ram = true;
                GB_update_ram_banks(gb);
            }
            // if we have rtc and in range, set the mapped rtc reg
            else if (GB_has_mbc_flags(gb, MBC_FLAGS_RTC) && value >= 0x08 && value <= 0x0C)
            {
                gb->cart.rtc_mapped_reg = value - 0x08;
                gb->cart.in_ram = false;
                GB_speed_hack_map_rtc_reg(gb);
            }
            break;

    // LATCH CLOCK DATA
        case 0x6: case 0x7:
            break;

        case 0xA: case 0xB:
            if (GB_has_mbc_flags(gb, MBC_FLAGS_RAM) && gb->cart.ram_enabled)
            {
                if (gb->cart.in_ram)
                {
                    gb->ram[(addr & 0x1FFF) + (0x2000 * gb->cart.ram_bank)] = value;
                }
                else if (GB_has_mbc_flags(gb, MBC_FLAGS_RTC))
                {
                    mbc3_rtc_write(gb, value);
                }
            }
            break;
    }
}

struct MBC_RomBankInfo mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank)
{
    struct MBC_RomBankInfo info = {0};
    const uint8_t* ptr = NULL;

    if (bank == 0)
    {
        ptr = gb->rom;
    }
    else
    {
        ptr = gb->rom + (gb->cart.rom_bank * 0x4000);
    }

    for (size_t i = 0; i < ARRAY_SIZE(info.entries); ++i)
    {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}

struct MBC_RamBankInfo mbc3_get_ram_bank(struct GB_Core* gb)
{
    if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled || !gb->cart.in_ram)
    {
        return mbc_setup_empty_ram();
    }

    struct MBC_RamBankInfo info = {0};

    const uint8_t* ptr = gb->ram + (0x2000 * gb->cart.ram_bank);

    for (size_t i = 0; i < ARRAY_SIZE(info.entries); ++i)
    {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}
