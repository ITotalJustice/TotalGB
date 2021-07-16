#ifndef GB_MBC_H
#define GB_MBC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


GB_INLINE void mbc_write(struct GB_Core *gb, uint16_t addr, uint8_t value);
GB_INLINE struct MBC_RomBankInfo mbc_get_rom_bank(struct GB_Core *gb, uint8_t bank);
GB_INLINE struct MBC_RamBankInfo mbc_get_ram_bank(struct GB_Core *gb);


GB_STATIC struct MBC_RamBankInfo mbc_setup_empty_ram(void);

GB_INLINE void mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
GB_STATIC struct MBC_RomBankInfo mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank);
GB_STATIC struct MBC_RamBankInfo mbc0_get_ram_bank(struct GB_Core* gb);

GB_INLINE void mbc1_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
GB_STATIC struct MBC_RomBankInfo mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank);
GB_STATIC struct MBC_RamBankInfo mbc1_get_ram_bank(struct GB_Core* gb);

GB_INLINE void mbc2_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
GB_STATIC struct MBC_RomBankInfo mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank);
GB_STATIC struct MBC_RamBankInfo mbc2_get_ram_bank(struct GB_Core* gb);

GB_INLINE void mbc3_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
GB_STATIC struct MBC_RomBankInfo mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank);
GB_STATIC struct MBC_RamBankInfo mbc3_get_ram_bank(struct GB_Core* gb);

GB_INLINE void mbc5_write(struct GB_Core* gb, uint16_t addr, uint8_t value);
GB_STATIC struct MBC_RomBankInfo mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank);
GB_STATIC struct MBC_RamBankInfo mbc5_get_ram_bank(struct GB_Core* gb);

#ifdef __cplusplus
}
#endif

#endif // GB_MBC_H
