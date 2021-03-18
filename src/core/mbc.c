#include "gb.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>

static const GB_U8 MBC_NO_RAM[0x2000];

GB_BOOL GB_get_cart_ram_size(GB_U8 type, GB_U32* size) {
	static const GB_U32 GB_CART_RAM_SIZES[6] = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
	if (type >= GB_ARR_SIZE(GB_CART_RAM_SIZES)) {
		return GB_FALSE;
	}
	*size = GB_CART_RAM_SIZES[type];
	return GB_TRUE;
}

static void GB_mbc0_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	UNUSED(gb); UNUSED(addr); UNUSED(value);
}

static const GB_U8* GB_mbc0_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.mbc.rom;
	}
	return gb->cart.mbc.rom + 0x4000;
}

static const GB_U8* GB_mbc0_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(gb); UNUSED(bank);
	return MBC_NO_RAM;
}

static void GB_mbc1_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	switch ((addr >> 12) & 0xF) {
	// RAM ENABLE
    case 0x0: case 0x1:
		gb->cart.mbc.ram_enabled = (!!(value & 0xA));
		GB_update_ram_banks(gb);
		break;
	// ROM BANK
    case 0x2: case 0x3: // ((value & 0x1F) + !(value & 0x1))
		gb->cart.mbc.rom_bank = value & 0x1F;
		if (!gb->cart.mbc.rom_bank) gb->cart.mbc.rom_bank = 1;
		GB_update_rom_banks(gb);
		break;
	// ROM / RAM BANK
    case 0x4: case 0x5:
		if (gb->cart.mbc.bank_mode) {
			gb->cart.mbc.ram_bank = value & 3;
			GB_update_ram_banks(gb);
		} else {
			gb->cart.mbc.rom_bank = (gb->cart.mbc.rom_bank & 0x1F) | ((value & 224));
			GB_update_rom_banks(gb);
		}
		break;
	// ROM / RAM MODE
    case 0x6: case 0x7:
		gb->cart.mbc.bank_mode = value & 1;
		break;
	// RAM BANK X
    case 0xA: case 0xB:
		if (!(gb->cart.mbc.flags & MBC_FLAGS_RAM) || !gb->cart.mbc.ram_enabled) {
			return;
		}
		gb->cart.mbc.ram[(addr & 0x1FFF) + (0x2000 * (gb->cart.mbc.bank_mode ? gb->cart.mbc.ram_bank : 0))] = value;
		break;
	default: __builtin_unreachable();
	}
}

static const GB_U8* GB_mbc1_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.mbc.rom;
	}
	
	return gb->cart.mbc.rom + (gb->cart.mbc.rom_bank * 0x4000);
}

static const GB_U8* GB_mbc1_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(bank);
	if (!(gb->cart.mbc.flags & MBC_FLAGS_RAM) || !gb->cart.mbc.ram_enabled) {
		return MBC_NO_RAM;
	}

	if (gb->cart.mbc.bank_mode) {
		return gb->cart.mbc.ram + (0x2000 * gb->cart.mbc.ram_bank);
	}
	return gb->cart.mbc.ram;
}

static void GB_mbc3_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) { 
    switch ((addr >> 12) & 0xF) {
	// RAM / RTC REGISTER ENABLE
	case 0x0: case 0x1:
		gb->cart.mbc.ram_enabled = (!!(value & 0xA));
		GB_update_ram_banks(gb);
		break;
	// ROM BANK
	case 0x2: case 0x3:
		gb->cart.mbc.rom_bank = value == 0 ? 1 : value;
		GB_update_rom_banks(gb);
		break;
	// RAM BANK / RTC REGISTER
	case 0x4: case 0x5:
		gb->cart.mbc.ram_bank = value;
		gb->cart.mbc.in_ram = value < 0x07;
		GB_update_ram_banks(gb);
		break;
	// LATCH CLOCK DATA
	case 0x6: case 0x7:
		break;
	case 0xA: case 0xB:
		if (gb->cart.mbc.in_ram) {
			gb->cart.mbc.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.mbc.ram_bank)] = value;
		}
		break;
	default: __builtin_unreachable();
    }
}

static const GB_U8* GB_mbc3_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.mbc.rom;
	}
	
	return gb->cart.mbc.rom + (gb->cart.mbc.rom_bank * 0x4000);
}

// todo: rtc support
static const GB_U8* GB_mbc3_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(bank);
	if (!(gb->cart.mbc.flags & MBC_FLAGS_RAM) || !gb->cart.mbc.ram_enabled || !gb->cart.mbc.in_ram) {
		return MBC_NO_RAM;
	}

	return gb->cart.mbc.ram + (0x2000 * gb->cart.mbc.ram_bank);
}

static void GB_mbc5_write(struct GB_Data* gb, GB_U16 addr, GB_U8 value) { 
    switch ((addr >> 12) & 0xF) {
	// RAM / RTC REGISTER ENABLE
	case 0x0: case 0x1:
		gb->cart.mbc.ram_enabled = (!!(value & 0xA));
		GB_update_ram_banks(gb);
		break;
	// ROM BANK LOW
	case 0x2:
		gb->cart.mbc.rom_bank = (gb->cart.mbc.rom_bank & 0xFF00) | value;
		GB_update_rom_banks(gb);
		break;
	// ROM BANK HIGH
	case 0x3:
		gb->cart.mbc.rom_bank = (gb->cart.mbc.rom_bank & 0x00FF) | (value << 8);
		GB_update_rom_banks(gb);
		break;
	// RAM BANK / RTC REGISTER
	case 0x4: case 0x5:
		gb->cart.mbc.ram_bank = value & 0xF;
		GB_update_ram_banks(gb);
		break;
	// LATCH CLOCK DATA
	case 0x6: case 0x7:
		break;
	case 0xA: case 0xB:
		if (gb->cart.mbc.ram_enabled) {
			gb->cart.mbc.ram[(addr & 0x1FFF) + (0x2000 * gb->cart.mbc.ram_bank)] = value;
		}
		break;
	default: __builtin_unreachable();
    }
}

static const GB_U8* GB_mbc5_get_rom_bank(struct GB_Data* gb, GB_U8 bank) {
	if (bank == 0) {
		return gb->cart.mbc.rom;
	}
	
	return gb->cart.mbc.rom + (gb->cart.mbc.rom_bank * 0x4000);
}

// todo: rtc support
static const GB_U8* GB_mbc5_get_ram_bank(struct GB_Data* gb, GB_U8 bank) {
	UNUSED(bank);
	if (!(gb->cart.mbc.flags & MBC_FLAGS_RAM) || !gb->cart.mbc.ram_enabled) {
		return MBC_NO_RAM;
	}

	return gb->cart.mbc.ram + (0x2000 * gb->cart.mbc.ram_bank);
}

struct GB_MbcSetupData {
	void (*write)(struct GB_Data *gb, GB_U16 addr, GB_U8 value);
	const GB_U8* (*get_rom_bank)(struct GB_Data *gb, GB_U8 bank);
	const GB_U8* (*get_ram_bank)(struct GB_Data *gb, GB_U8 bank);
	GB_U16 starting_rom_bank;
	GB_U8 flags;
};

// i think this is c99 only sadly
static const struct GB_MbcSetupData MBC_SETUP_DATA[0x100] = {
	// MBC0
	[0x00] = {GB_mbc0_write, GB_mbc0_get_rom_bank, GB_mbc0_get_ram_bank, 0, MBC_FLAGS_NONE},
	// MBC1
	[0x01] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_NONE},
	[0x02] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_RAM},
	[0x03] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	// MBC3
	[0x10] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 0, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY | MBC_FLAGS_RTC},
	[0x11] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 0, MBC_FLAGS_NONE},
	[0x13] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 0, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	// MBC5
	[0x19] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_NONE},
	[0x1A] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_RAM},
	[0x1B] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY | MBC_FLAGS_RTC},
	[0x1C] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_NONE},
	[0x1D] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_RAM},
	[0x1E] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 0, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY | MBC_FLAGS_RTC},
};

GB_BOOL GB_setup_mbc(struct GB_MbcData* mbc, const struct GB_CartHeader* header) {
	if (!mbc || !header) {
		return GB_FALSE;
	}

	const struct GB_MbcSetupData* data = &MBC_SETUP_DATA[header->cart_type];
	// this is a check to see if we have data.
	// i could check only 1 func, but just in case i check all 3.
	if (!data->write || !data->get_rom_bank || !data->get_ram_bank) {
		printf("MBC NOT IMPLEMENTED: 0x%02X\n", header->cart_type);
		assert(0);
		return GB_FALSE;
	}

	mbc->write = data->write;
	mbc->get_rom_bank = data->get_rom_bank;
	mbc->get_ram_bank = data->get_ram_bank;
	mbc->rom_bank = data->starting_rom_bank;
	mbc->flags = data->flags;

	// todo: setup more flags.
	if (mbc->flags & MBC_FLAGS_RAM) {
		if (!GB_get_cart_ram_size(header->ram_size, &mbc->ram_size)) {
			printf("rom has ram but the size entry in header is invalid! %u\n", header->ram_size);
			return GB_FALSE;
		}
	}

	return GB_TRUE;
}
