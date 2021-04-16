#include "core/mbc/common.h"


void GB_mbc0_write(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	GB_UNUSED(gb); GB_UNUSED(addr); GB_UNUSED(value);
}

struct MBC_BankInfo GB_mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
	if (bank == 0) {
		return (struct MBC_BankInfo) {
			.ptr = gb->cart.rom,
			.mask = 0x1FFF
		};
	}
	else
	{
		return (struct MBC_BankInfo) {
			.ptr = gb->cart.rom + 0x4000,
			.mask = 0x1FFF
		};
	}
}

struct MBC_BankInfo GB_mbc0_get_ram_bank(struct GB_Core* gb) {
	GB_UNUSED(gb);
	
	static const uint8_t MBC_NO_RAM[1] = { 0xFF} ;

	return (struct MBC_BankInfo) {
		.ptr = MBC_NO_RAM,
		.mask = 0x1
	};
}
