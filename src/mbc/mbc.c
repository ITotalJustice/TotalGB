#include "../gb.h"
#include "../internal.h"
#include "mbc.h"

#include <string.h>
#include <assert.h>


struct MbcInfo
{
    uint8_t type;
    uint8_t flags;
};

void mbc_write(struct GB_Core *gb, uint16_t addr, uint8_t value)
{
    switch (gb->cart.type)
    {
        case GB_MbcType_0: mbc0_write(gb, addr, value); break;
        case GB_MbcType_1: mbc1_write(gb, addr, value); break;
        case GB_MbcType_2: mbc2_write(gb, addr, value); break;
        case GB_MbcType_3: mbc3_write(gb, addr, value); break;
        case GB_MbcType_5: mbc5_write(gb, addr, value); break;
    }
}

struct MBC_RomBankInfo mbc_get_rom_bank(struct GB_Core *gb, uint8_t bank)
{
    if (gb->callback.rom_bank)
    {
        struct MBC_RomBankInfo info = {0};

        if (gb->callback.rom_bank(gb->callback.user_rom_bank, &info, gb->cart.type, bank * gb->cart.rom_bank))
        {
            return info;
        }
    }

    switch (gb->cart.type)
    {
        case GB_MbcType_0: return mbc0_get_rom_bank(gb, bank);
        case GB_MbcType_1: return mbc1_get_rom_bank(gb, bank);
        case GB_MbcType_2: return mbc2_get_rom_bank(gb, bank);
        case GB_MbcType_3: return mbc3_get_rom_bank(gb, bank);
        case GB_MbcType_5: return mbc5_get_rom_bank(gb, bank);
    }

    UNREACHABLE((struct MBC_RomBankInfo){0});
}

struct MBC_RamBankInfo mbc_get_ram_bank(struct GB_Core *gb)
{
    switch (gb->cart.type)
    {
        case GB_MbcType_0: return mbc0_get_ram_bank(gb);
        case GB_MbcType_1: return mbc1_get_ram_bank(gb);
        case GB_MbcType_2: return mbc2_get_ram_bank(gb);
        case GB_MbcType_3: return mbc3_get_ram_bank(gb);
        case GB_MbcType_5: return mbc5_get_ram_bank(gb);
    }

    UNREACHABLE((struct MBC_RamBankInfo){0});
}

// NOTE: this assumes that the rest of the entries will be zero init!
static const struct MbcInfo MBC_INFO[0x100] =
{
    // MBC0
    [0x00] = { .type = GB_MbcType_0, .flags = MBC_FLAGS_NONE },
    // MBC1
    [0x01] = { .type = GB_MbcType_1, .flags = MBC_FLAGS_NONE },
    [0x02] = { .type = GB_MbcType_1, .flags = MBC_FLAGS_RAM },
    [0x03] = { .type = GB_MbcType_1, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY },
    // MBC2
    [0x05] = { .type = GB_MbcType_2, .flags = MBC_FLAGS_RAM },
    [0x06] = { .type = GB_MbcType_2, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY },
    // MBC3
    [0x0F] = { .type = GB_MbcType_3, .flags = MBC_FLAGS_BATTERY | MBC_FLAGS_RTC },
    [0x10] = { .type = GB_MbcType_3, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY | MBC_FLAGS_RTC },
    [0x11] = { .type = GB_MbcType_3, .flags = MBC_FLAGS_NONE },
    [0x13] = { .type = GB_MbcType_3, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY },
    // MBC5
    [0x19] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_NONE },
    [0x1A] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_RAM },
    [0x1B] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY },
    [0x1C] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_RUMBLE },
    [0x1D] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_RAM | MBC_FLAGS_RUMBLE },
    [0x1E] = { .type = GB_MbcType_5, .flags = MBC_FLAGS_RAM | MBC_FLAGS_BATTERY },
    // todo: multicart, huc1, huc3
};

struct MBC_RamBankInfo mbc_setup_empty_ram(void)
{
    static const uint8_t MBC_NO_RAM = 0xFF;

    return (struct MBC_RamBankInfo)
    {
        .entries[0] =
        {
            .ptr = &MBC_NO_RAM,
            .mask = 0,
        },
        .entries[1] =
        {
            .ptr = &MBC_NO_RAM,
            .mask = 0,
        }
    };
}

static bool is_ascii_char_valid(const char c)
{
    // always upper case! ignore lower case ascii
    return c >= ' ' && c <= '_';
}

int GB_get_rom_name_from_header(const struct GB_CartHeader* header, struct GB_CartName* name)
{
    // in later games, including all gbc games, the title area was
    // actually reduced in size from 16 bytes down to 15, then 11.
    // as all titles are UPPER_CASE ASCII, it is easy to range check each
    // char and see if its valid, once the end is found, mark the next char NULL.
    // NOTE: it seems that spaces are also valid!

    // set entrie name to NULL
    memset(name, 0, sizeof(struct GB_CartName));

    // manually copy the name using range check as explained above...
    for (size_t i = 0; i < ARRAY_SIZE(header->title); ++i)
    {
        const char c = header->title[i];

        if (is_ascii_char_valid(c) == false)
        {
            break;
        }

        name->name[i] = c;
    }

    return 0;
}

int GB_get_rom_name(const struct GB_Core* gb, struct GB_CartName* name)
{
    const struct GB_CartHeader* header = GB_get_rom_header_ptr(gb);

    return GB_get_rom_name_from_header(header, name);
}

bool GB_get_cart_ram_size(uint8_t type, uint32_t* size)
{
    // i think that more ram sizes are valid, however
    // i have yet to see a ram size bigger than this...
    const uint32_t GB_CART_RAM_SIZES[6] =
    {
        [0] = GB_SAVE_SIZE_NONE   /*0*/,
        [1] = GB_SAVE_SIZE_1      /*0x800*/,
        [2] = GB_SAVE_SIZE_2      /*0x2000*/,
        [3] = GB_SAVE_SIZE_3      /*0x8000*/,
        [4] = GB_SAVE_SIZE_4      /*0x20000*/,
        [5] = GB_SAVE_SIZE_5      /*0x10000*/,
    };

    assert(type < ARRAY_SIZE(GB_CART_RAM_SIZES) && "OOB type access!");

    if (type >= ARRAY_SIZE(GB_CART_RAM_SIZES))
    {
        GB_log_fatal("invalid ram size type! %u\n", type);
        return false;
    }

    if (type == 5 || type == 4)
    {
        GB_log("ram is of type GB_SAVE_SIZE_5, finally found a game that uses this!\n");
        assert(type != 5);
        return false;
    }

    *size = GB_CART_RAM_SIZES[type];
    return true;
}

bool GB_get_mbc_flags(uint8_t cart_type, uint8_t* flags_out)
{
    const struct MbcInfo* info = &MBC_INFO[cart_type];

    if (!info->type)
    {
        return false;
    }

    *flags_out = info->flags;

    return true;
}

bool GB_setup_mbc(struct GB_Cart* mbc, const struct GB_CartHeader* header)
{
    if (!mbc || !header)
    {
        return false;
    }

    // this won't fail because the type is 8-bit and theres 0x100 entries.
    // though the data inside can be NULL, but this is checked next...
    const struct MbcInfo* info = &MBC_INFO[header->cart_type];

    // types start at > 0, if 0, then this mbc is invalid
    if (!info->type)
    {
        GB_log("MBC NOT IMPLEMENTED: 0x%02X\n", header->cart_type);
        assert(0);
        return false;
    }

    mbc->type = info->type;
    mbc->flags = info->flags;

    // todo: create mbcx_init() functions
    if (mbc->type == GB_MbcType_0)
    {
        mbc->rom_bank = 0;
        mbc->rom_bank_lo = 0;
    }
    else
    {
        mbc->rom_bank = 1;
        mbc->rom_bank_lo = 1;
    }

    // setup max rom banks
    // this can never be 0 as rom size is already set before.
    assert(mbc->rom_size > 0 && "you changed where rom size is set!");
    mbc->rom_bank_max = mbc->rom_size / 0x4000;

    // todo: setup more flags.
    if (mbc->flags & MBC_FLAGS_RAM)
    {
        // check if mbc2, if so, manually set the ram size!
        if (header->cart_type == 0x5 || header->cart_type == 0x6)
        {
            mbc->ram_size = 0x200;
        }
        // otherwise get the ram size via a LUT
        else if (!GB_get_cart_ram_size(header->ram_size, &mbc->ram_size))
        {
            GB_log("rom has ram but the size entry in header is invalid! %u\n", header->ram_size);
            return false;
        }
        else
        {
            mbc->ram_bank_max = mbc->ram_size / 0x2000;
        }

        // check that the size (if any) returned is within range of the
        // maximum ram size.
        if (mbc->ram_size > mbc->max_ram_size)
        {
            GB_log("cart-ram size is too big for the maximum size set! got: %u max: %zu", mbc->ram_size, mbc->max_ram_size);
            return false;
        }
    }

    return true;
}
