#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

static void GB_mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	GB_UNUSED(gb); GB_UNUSED(addr); GB_UNUSED(value);
}

static const uint8_t* GB_mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	return gb->cart.rom + 0x4000;
}

static const uint8_t* GB_mbc0_get_ram_bank(struct GB_Core* gb, uint8_t bank) {
	GB_UNUSED(gb); GB_UNUSED(bank);
	return MBC_NO_RAM;
}

#ifdef __cplusplus
}
#endif
