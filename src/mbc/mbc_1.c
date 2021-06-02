#include "../internal.h"
#include "mbc.h"

#if GB_SRC_INCLUDE

#include <assert.h>


void mbc1_write(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    switch ((addr >> 12) & 0xF)
    {
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
            gb->cart.rom_bank = ((gb->cart.rom_bank_hi << 5) | (gb->cart.rom_bank_lo)) % gb->cart.rom_bank_max;
            GB_update_rom_banks(gb);
        } break;

    // ROM / RAM BANK
        case 0x4: case 0x5:
            if (gb->cart.rom_bank_max > 18)
            {
                gb->cart.rom_bank_hi = value & 0x3;
                gb->cart.rom_bank = ((gb->cart.rom_bank_hi << 5) | gb->cart.rom_bank_lo) % gb->cart.rom_bank_max;
                GB_update_rom_banks(gb);
            }

            if (gb->cart.bank_mode)
            {
                if ((gb->cart.flags & MBC_FLAGS_RAM))
                {
                    gb->cart.ram_bank = (value & 0x3) % gb->cart.ram_bank_max;
                    GB_update_ram_banks(gb);
                }
            }
            break;

    // ROM / RAM MODE
        case 0x6: case 0x7:
            gb->cart.bank_mode = value & 0x1;

            if (gb->cart.rom_bank_max > 18)
            {
                if (gb->cart.bank_mode == 0)
                {
                    gb->cart.rom_bank = gb->cart.rom_bank_lo;
                }
            }
            GB_update_rom_banks(gb);
            break;

    // RAM BANK X
        case 0xA: case 0xB:
            if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled)
            {
                return;
            }
            gb->cart.ram[(addr & 0x1FFF) + (0x2000 * (gb->cart.bank_mode == 1 ? gb->cart.ram_bank : 0))] = value;
            break;
    }
}

struct MBC_RomBankInfo mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank)
{
    struct MBC_RomBankInfo info = {0};
    const uint8_t* ptr = NULL;

    if (bank == 0)
    {
        if (gb->cart.rom_bank_max > 18 && gb->cart.bank_mode == 1)
        {
            uint16_t b = (gb->cart.rom_bank_hi << 5);
            if (b > gb->cart.rom_bank_max)
            {
                b %= gb->cart.rom_bank_max;
            }
            ptr = gb->cart.rom + (b * 0x4000);
        } else {
            ptr = gb->cart.rom;
        }
    }
    else {
        ptr = gb->cart.rom + (gb->cart.rom_bank * 0x4000);
    }

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i)
    {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}

struct MBC_RamBankInfo mbc1_get_ram_bank(struct GB_Core* gb)
{
    if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled)
    {
        return mbc_setup_empty_ram();
    }

    struct MBC_RamBankInfo info = {0};

    // in mode 0, always read from bank 0
    const uint8_t* ptr = gb->cart.ram + (0x2000 * (gb->cart.bank_mode == 1 ? gb->cart.ram_bank : 0));

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i)
    {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}

#endif //GB_SRC_INCLUDE
