#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"


void GB_mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
const uint8_t* GB_mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank);
const uint8_t* GB_mbc0_get_ram_bank(struct GB_Core* gb, uint8_t bank);

void GB_mbc1_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
const uint8_t* GB_mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank);
const uint8_t* GB_mbc1_get_ram_bank(struct GB_Core* gb, uint8_t bank);

void GB_mbc2_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
const uint8_t* GB_mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank);
const uint8_t* GB_mbc2_get_ram_bank(struct GB_Core* gb, uint8_t bank);

void GB_mbc3_write(struct GB_Core* gb, uint16_t addr, uint8_t value); 
const uint8_t* GB_mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank);
const uint8_t* GB_mbc3_get_ram_bank(struct GB_Core* gb, uint8_t bank);

void GB_mbc5_write(struct GB_Core* gb, uint16_t addr, uint8_t value); 
const uint8_t* GB_mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank);
const uint8_t* GB_mbc5_get_ram_bank(struct GB_Core* gb, uint8_t bank);

#ifdef __cplusplus
}
#endif