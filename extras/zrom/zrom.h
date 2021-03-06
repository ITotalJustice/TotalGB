#ifndef _ZROM_H_
#define _ZROM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <gb.h>


enum ZromEntryFlag
{
    ZromEntryFlag_COMPRESSED = 1 << 0,
    // some banks don't compress at all, and end
    // up larger than 16KiB!.
    // for those, we keep the bank uncompressed
    // which allows for memcpy speed!
    ZromEntryFlag_UNCOMPRESSED = 1 << 1,
};

enum
{
    ZROM_MAGIC = 0xFACADE,
    ZROM_MAX_BANKS = 0x100,
    ZROM_BANK_SIZE = 1024 * 16,
};

struct ZromHeader
{
    uint32_t magic;
    uint16_t banks;
    uint16_t reserved;
};

struct ZromBankEntry
{
    uint32_t offset;
    uint16_t size;
    uint8_t flags;
    uint8_t reserved;
};

struct Zrom
{
    struct GB_Core* gb;

    const uint8_t* rom_data;
    size_t rom_size;

    uint8_t* pool;
    size_t pool_count;

    struct ZromHeader header;
    struct ZromBankEntry entries[ZROM_MAX_BANKS];
    
    uint8_t last_used[ZROM_MAX_BANKS];
    uint8_t last_used_count;

    uint8_t pool_idx[ZROM_MAX_BANKS];
    
    bool slots[ZROM_MAX_BANKS];
};

// this can be called before `zrom_init()`
// checks if the zrom header is found, no futher checks are performed such
// as is the rom itself is valid!
GBAPI bool is_zrom(const uint8_t* data, size_t size);

GBAPI bool zrom_init(struct Zrom* z, struct GB_Core* gb, uint8_t* pool, size_t size);
GBAPI void zrom_exit(struct Zrom* z);
GBAPI bool zrom_loadrom_compressed(struct Zrom* z, const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // _ZROM_H_
