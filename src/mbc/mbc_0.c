#include "../internal.h"
#include "mbc.h"

#if GB_SRC_INCLUDE

#include <assert.h>


void mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    GB_UNUSED(gb); GB_UNUSED(addr); GB_UNUSED(value);
}

struct MBC_RomBankInfo mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank)
{
    struct MBC_RomBankInfo info = {0};
    const uint8_t* ptr = NULL;

    if (bank == 0)
    {
        ptr = gb->cart.rom;
    }
    else
    {
        ptr = gb->cart.rom + 0x4000;
    }

    for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i)
    {
        info.entries[i].ptr = ptr + (0x1000 * i);
        info.entries[i].mask = 0x0FFF;
    }

    return info;
}

struct MBC_RamBankInfo mbc0_get_ram_bank(struct GB_Core* gb)
{
    GB_UNUSED(gb);

    return mbc_setup_empty_ram();
}

#endif // GB_SRC_INCLUDE
