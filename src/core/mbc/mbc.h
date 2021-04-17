#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"


struct MBC_RamBankInfo mbc_setup_empty_ram(void);

void GB_mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
struct MBC_RomBankInfo GB_mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank);
struct MBC_RamBankInfo GB_mbc0_get_ram_bank(struct GB_Core* gb);

void GB_mbc1_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
struct MBC_RomBankInfo GB_mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank);
struct MBC_RamBankInfo GB_mbc1_get_ram_bank(struct GB_Core* gb);

void GB_mbc2_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
struct MBC_RomBankInfo GB_mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank);
struct MBC_RamBankInfo GB_mbc2_get_ram_bank(struct GB_Core* gb);

void GB_mbc3_write(struct GB_Core* gb, uint16_t addr, uint8_t value); 
struct MBC_RomBankInfo GB_mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank);
struct MBC_RamBankInfo GB_mbc3_get_ram_bank(struct GB_Core* gb);

void GB_mbc5_write(struct GB_Core* gb, uint16_t addr, uint8_t value); 
struct MBC_RomBankInfo GB_mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank);
struct MBC_RamBankInfo GB_mbc5_get_ram_bank(struct GB_Core* gb);

#ifdef __cplusplus
}
#endif
