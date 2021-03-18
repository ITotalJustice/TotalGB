#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

static void GB_mbc0_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	UNUSED(gb); UNUSED(addr); UNUSED(value);
}

static const GB_U8* GB_mbc0_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	return gb->cart.rom + 0x4000;
}

static const GB_U8* GB_mbc0_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(gb); UNUSED(bank);
	return MBC_NO_RAM;
}

#ifdef __cplusplus
}
#endif
