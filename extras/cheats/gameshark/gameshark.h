// this uses stdlib for malloc and free!
// please call gameshark_exit() before main exit!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


typedef uint32_t gameshark_id_t;
enum { GAMESHARK_INAVLID_ID = 0 };

enum GamesharkType
{
    GamesharkType_WRITE,
    GamesharkType_WRITE_WITH_RAM_BANK_CHANGE,
    GamesharkType_WRITE_WITH_WRAM_BANK_CHANGE,
};

struct CheatEntry
{
    struct CheatEntry* next;
    gameshark_id_t id;
    enum GamesharkType type;

    uint16_t addr;
    uint8_t value;
    uint8_t bank;

    bool enabled;
};

struct Gameshark
{
    struct CheatEntry* entries;
    size_t count;
};

// typedef uint8_t (*gs_read_t)(void* user, uint16_t addr);
typedef void    (*gs_write_t)(void* user, uint16_t addr, uint8_t value);
// typedef uint8_t (*gs_get_ram_bank_t)(void* user);
// typedef uint8_t (*gs_get_wram_bank_t)(void* user);
typedef void    (*gs_set_ram_bank_t)(void* user, uint8_t bank);
typedef void    (*gs_set_wram_bank_t)(void* user, uint8_t bank);


bool gameshark_init(struct Gameshark* gs);
void gameshark_exit(struct Gameshark* gs);

gameshark_id_t gameshark_add_cheat(struct Gameshark* gs, const char* s);
void gameshark_remove_cheat(struct Gameshark* gs, gameshark_id_t id);
void gameshark_reset(struct Gameshark* gs);

bool gameshark_enable_cheat(struct Gameshark* gs, gameshark_id_t id);
bool gameshark_disable_cheat(struct Gameshark* gs, gameshark_id_t id);

// gameshark cheats are applied at vblank
void gameshark_on_vblank(struct Gameshark* gs, void* user,
    gs_write_t write,
    gs_set_ram_bank_t set_ram_bank,
    gs_set_wram_bank_t set_wram_bank
);

#ifdef __cplusplus
}
#endif
