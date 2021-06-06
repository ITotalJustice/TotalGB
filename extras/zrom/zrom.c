#include "zrom.h"

#include "lz4.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


static bool mbc_common_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc0_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc1_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc2_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc3_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc5_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool uncompress_bank_to_pool(struct Zrom* z, size_t bank);


static bool mbc_common_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    if (!uncompress_bank_to_pool(z, bank * z->gb->cart.rom_bank))
    {
        return false;
    }

    const uint8_t* ptr = bank == 0 ? z->rom_data : z->bank_buffer;

    for (size_t i = 0; i < sizeof(info->entries) / sizeof(info->entries[0]); ++i)
    {
        info->entries[i].ptr = ptr + (0x1000 * i);
        info->entries[i].mask = 0x0FFF;
    }

    return true;
}

static bool mbc0_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    return mbc_common_get_rom_bank(z, info, bank);
}

// NOTE: this is a stub, it wont work yet!!!!!!!!
static bool mbc1_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    (void)z; (void)info; (void)bank;
    return false;
}

static bool mbc2_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    return mbc_common_get_rom_bank(z, info, bank);
}

static bool mbc3_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    return mbc_common_get_rom_bank(z, info, bank);
}

static bool mbc5_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    return mbc_common_get_rom_bank(z, info, bank);
}

static bool uncompress_bank_to_pool(struct Zrom* z, size_t bank)
{
    // bank zero is always uncompressed
    if (bank == 0)
    {
        return true;
    }

    --bank;

    // if already compressed
    if (z->slots[bank])
    {
        return true;
    }

    const struct ZromBankEntry entry = z->entries[bank];

    if (entry.flags & ZromEntryFlag_COMPRESSED)
    {
        LZ4_decompress_safe((const char*)z->rom_data + entry.offset, (char*)z->bank_buffer, entry.size, sizeof(z->bank_buffer));
    }
    else if (entry.flags & ZromEntryFlag_UNCOMPRESSED)
    {
        memcpy((char*)z->bank_buffer, (const char*)z->rom_data + entry.offset, entry.size);
    }
    else
    {
        assert(0 && "missing zrom flags...");
        return false;
    }

    z->slots[z->bank] = false;
    z->bank = bank;
    z->slots[bank] = true;

    return true;
}

static bool on_rom_bank_callback(void* user, struct MBC_RomBankInfo* info, enum GB_MbcType type, uint8_t bank)
{
    // printf("on rom callback, %u\n", bank);
    struct Zrom* z = (struct Zrom*)user;

    switch (type)
    {
        case GB_MbcType_0: return mbc0_get_rom_bank(z, info, bank);
        case GB_MbcType_1: return mbc1_get_rom_bank(z, info, bank);
        case GB_MbcType_2: return mbc2_get_rom_bank(z, info, bank);
        case GB_MbcType_3: return mbc3_get_rom_bank(z, info, bank);
        case GB_MbcType_5: return mbc5_get_rom_bank(z, info, bank);  
    }

    return false;
}

bool zrom_init(struct Zrom* z, struct GB_Core* gb)
{
    memset(z, 0, sizeof(struct Zrom));

    z->gb = gb;

    GB_set_rom_bank_callback(z->gb, on_rom_bank_callback, z);

    return true;
}

void zrom_exit(struct Zrom* z)
{
    memset(z, 0, sizeof(struct Zrom));
}

bool zrom_loadrom_compressed(struct Zrom* z, const uint8_t* data, size_t size)
{
    z->rom_data = data;

    memset(z->slots, 0, sizeof(z->slots));
    memset(z->entries, 0, sizeof(z->entries));
    memset(z->bank_buffer, 0, sizeof(z->bank_buffer));
    memset(&z->bank, 0, sizeof(z->bank));

    const uint8_t* ptr = data + ZROM_BANK_SIZE;

    memcpy(&z->header, ptr, sizeof(z->header));

    printf("header magic 0x%04X\n", z->header.magic);
    printf("header banks %u\n", z->header.banks);

    memcpy(z->entries, ptr + sizeof(z->header), z->header.banks * sizeof(struct ZromBankEntry));

    // load bank 1 immediatly
    if (!uncompress_bank_to_pool(z, 1))
    {
        return false;
    }

    return GB_loadrom(z->gb, z->rom_data, size);
}
