#include "zrom.h"
#include <lz4.h>
#include <string.h>


#ifndef ZROM_DEBUG
    #define ZROM_DEBUG 0
#endif

#if ZROM_DEBUG
    #include <stdio.h>
    #include <assert.h>
    #define ZROM_log(...) fprintf(stdout, __VA_ARGS__)
    #define ZROM_log_fatal(...) do { fprintf(stdout, __VA_ARGS__); assert(0); } while(0)
#else
    #define ZROM_log(...)
    #define ZROM_log_fatal(...)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))


static bool mbc_common_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc0_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc1_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc2_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc3_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool mbc5_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank);
static bool uncompress_bank_to_pool(struct Zrom* z, uint8_t bank);


static bool mbc_common_get_rom_bank(struct Zrom* z, struct MBC_RomBankInfo* info, uint8_t bank)
{
    if (!uncompress_bank_to_pool(z, bank))
    {
        return false;
    }

    const uint8_t* ptr = z->rom_data;

    if (bank)
    {
        ptr = z->pool + (z->pool_idx[bank - 1] * ZROM_BANK_SIZE);
    }

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

static bool uncompress_bank_to_pool(struct Zrom* z, uint8_t bank)
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

        for (size_t i = 0; i < z->last_used_count; ++i)
        {
            if (z->last_used[i] == bank)
            {
                // break early if bank is in the first slot
                if (i == 0)
                {
                    return true;
                }

                memmove(z->last_used + 1, z->last_used, i);
                z->last_used[0] = bank;
                return true;
            }
        }

        ZROM_log_fatal("[ZROM] this shouldn't be reachable!!!\n");
        return true;
    }

    // if we uncompressed max banks already, then we need to
    // uncompress this new bank over an old bank.
    if (z->last_used_count == (z->pool_count))
    {
        // check which is the last used bank.
        const uint8_t old_bank = z->last_used[z->last_used_count - 1];
        // update the next free bank slot
        z->pool_idx[bank] = z->pool_idx[old_bank];
        // update slot array to say that it is no longer used!
        z->slots[old_bank] = false;

        ZROM_log("\t[ZROM] bank miss! old_bank: %u new_bank: %u\n", old_bank, bank);
    }
    else
    {
        ZROM_log("\t[ZROM] NEW Bank! bank: %u count: %u\n", bank, z->last_used_count);

        z->pool_idx[bank] = z->last_used_count;
        z->last_used_count = MIN((size_t)z->last_used_count + 1, z->pool_count);
    }

    uint8_t* next_free_bank = z->pool + (z->pool_idx[bank] * ZROM_BANK_SIZE);

    // new bank, not compressed
    memmove(z->last_used + 1, z->last_used, z->last_used_count);
    z->last_used[0] = bank;

    const struct ZromBankEntry entry = z->entries[bank];

    if (entry.flags & ZromEntryFlag_COMPRESSED)
    {
        LZ4_decompress_safe((const char*)z->rom_data + entry.offset, (char*)next_free_bank, entry.size, ZROM_BANK_SIZE);
    }
    else if (entry.flags & ZromEntryFlag_UNCOMPRESSED)
    {
        memcpy((char*)next_free_bank, (const char*)z->rom_data + entry.offset, entry.size);
    }
    else
    {
        ZROM_log_fatal("[ZROM] missing zrom flags...\n");
        return false;
    }

    z->slots[bank] = true;

    return true;
}

static bool on_rom_bank_callback(void* user, struct MBC_RomBankInfo* info, enum GB_MbcType type, uint8_t bank)
{
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

bool is_zrom(const uint8_t* data, size_t size)
{
    if (size < (ZROM_BANK_SIZE + sizeof(struct ZromHeader)))
    {
        return false;
    }

    const struct ZromHeader* header = (const struct ZromHeader*)(data + ZROM_BANK_SIZE);

    return header->magic == ZROM_MAGIC;
}

bool zrom_init(struct Zrom* z, struct GB_Core* gb, uint8_t* pool, size_t size)
{
    // size must be a multiple of ZROM_BANK_SIZE!
    if (!size || size % ZROM_BANK_SIZE)
    {
        return false;
    }

    memset(z, 0, sizeof(struct Zrom));

    z->gb = gb;
    z->pool = pool;
    z->pool_count = size / ZROM_BANK_SIZE;

    GB_set_rom_bank_callback(z->gb, on_rom_bank_callback, z);

    return true;
}

void zrom_exit(struct Zrom* z)
{
    memset(z, 0, sizeof(struct Zrom));
}

bool zrom_loadrom_compressed(struct Zrom* z, const uint8_t* data, size_t size)
{
    if (!is_zrom(data, size))
    {
        return false;
    }

    z->rom_data = data;

    memset(z->slots, 0, sizeof(z->slots));
    memset(z->entries, 0, sizeof(z->entries));
    memset(z->last_used, 0, sizeof(z->last_used));
    memset(z->pool_idx, 0, sizeof(z->pool_idx));
    memset(&z->last_used_count, 0, sizeof(z->last_used_count));

    const uint8_t* ptr = data + ZROM_BANK_SIZE;

    memcpy(&z->header, ptr, sizeof(z->header));

    ZROM_log("header magic 0x%04X\n", z->header.magic);
    ZROM_log("header banks %u\n", z->header.banks);

    memcpy(z->entries, ptr + sizeof(z->header), z->header.banks * sizeof(struct ZromBankEntry));

    // load bank 1 immediatly
    if (!uncompress_bank_to_pool(z, 1))
    {
        return false;
    }

    return GB_loadrom(z->gb, z->rom_data, size);
}
