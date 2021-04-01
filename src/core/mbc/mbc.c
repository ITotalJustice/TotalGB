#include "core/gb.h"
#include "core/internal.h"
#include "core/mbc/mbc.h"
#include "core/mbc/common.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

// this is extern in mbc/common.h
// this is used when either the game has no ram or ram is disabled
// and the game tries to read from the ram access area.
// because of how my memory reads are setup, array pointers are
// saved and index for a read.
// checking for NULL would be too slow of each read.
// because of this, this empty array is used as a saved pointer.
// writes are manually checked for now, so this can stay const!

// TODO: this can be const, however, i need to manually set the entire
// array to 0xFF, atm im using memset but this can be set at compile-time!
uint8_t MBC_NO_RAM[0x2000];

struct GB_MbcInterface {
	// function callbacks
	void (*write)(struct GB_Core *gb, uint16_t addr, uint8_t value);
	const uint8_t* (*get_rom_bank)(struct GB_Core *gb, uint8_t bank);
	const uint8_t* (*get_ram_bank)(struct GB_Core *gb, uint8_t bank);
	// all normal mbc's start we rombank == 1
	// however, other mbcs (HUC1) seem to start at 0
	uint8_t starting_rom_bank;
	// ram, rtc, battery etc...
	uint8_t flags;
};

// this data is indexed using the [cart_type] member from the
// cart header struct.
static const struct GB_MbcInterface MBC_SETUP_DATA[0x100] = {
	// MBC0
	[0x00] = {GB_mbc0_write, GB_mbc0_get_rom_bank, GB_mbc0_get_ram_bank, 0, MBC_FLAGS_NONE},
	// MBC1
	[0x01] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_NONE},
	[0x02] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_RAM},
	[0x03] = {GB_mbc1_write, GB_mbc1_get_rom_bank, GB_mbc1_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	// MBC2
	[0x05] = {GB_mbc2_write, GB_mbc2_get_rom_bank, GB_mbc2_get_ram_bank, 1, MBC_FLAGS_RAM},
	[0x06] = {GB_mbc2_write, GB_mbc2_get_rom_bank, GB_mbc2_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	// MBC3
	[0x0F] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 1, MBC_FLAGS_BATTERY | MBC_FLAGS_RTC},
	[0x10] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY | MBC_FLAGS_RTC},
	[0x11] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 1, MBC_FLAGS_NONE},
	[0x13] = {GB_mbc3_write, GB_mbc3_get_rom_bank, GB_mbc3_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	// MBC5
	[0x19] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_NONE},
	[0x1A] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_RAM},
	[0x1B] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
	[0x1C] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_RUMBLE},
	[0x1D] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_RUMBLE},
	[0x1E] = {GB_mbc5_write, GB_mbc5_get_rom_bank, GB_mbc5_get_ram_bank, 1, MBC_FLAGS_RAM | MBC_FLAGS_BATTERY},
};

static bool is_ascii_char_valid(const char c) {
	// always uppercase
	if (c >= 'A' && c <= 'Z') {
		return true;
	}

	if (c >= '0' && c <= '9') {
		return true;
	}

	// vaid symbols
	static const char symbols[] = {
		'?', '!', '-', '_', ' ',
	};

	// check against the symbol table
	for (size_t i = 0; i < GB_ARR_SIZE(symbols); ++i) {
		// try and find 1 valid match
		if (c == symbols[i]) {
			return true;
		}
	}

	return false;
}

int GB_get_rom_name_from_header(const struct GB_CartHeader* header, struct GB_CartName* name) {
	// in later games, including all gbc games, the title area was
	// actually reduced in size from 16 bytes down to 15, then 11.
	// as all titles are UPPER_CASE ASCII, it is easy to range check each
	// char and see if its valid, once the end is found, mark the next char NULL.
	// NOTE: it seems that spaces are also valid!

	// set entrie name to NULL
	memset(name, 0, sizeof(struct GB_CartName));

	// manually copy the name using range check as explained above...
	for (size_t i = 0; i < sizeof(header->title); ++i) {
		const char c = header->title[i];
		
		if (is_ascii_char_valid(c) == false) {
			break;
		}
		
		name->name[i] = c;
	}

	return 0;
}

int GB_get_rom_name(const struct GB_Core* gb, struct GB_CartName* name) {
	assert(gb && name);

	const struct GB_CartHeader* header = GB_get_rom_header_ptr(gb);
	assert(header);

	return GB_get_rom_name_from_header(header, name);
}

bool GB_get_cart_ram_size(uint8_t type, uint32_t* size) {
	// i think that more ram sizes are valid, however
	// i have yet to see a ram size bigger than this...
	static const uint32_t GB_CART_RAM_SIZES[6] = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
	
	assert(type < GB_ARR_SIZE(GB_CART_RAM_SIZES) && "OOB type access!");

	if (type >= GB_ARR_SIZE(GB_CART_RAM_SIZES)) {
		return false;
	}

	if (type == 1) {
		printf("ram size is 0x800, this will break mapped ram!\n");
		assert(type != 1);
		return false;
	}

	*size = GB_CART_RAM_SIZES[type];
	return true;
}

bool GB_setup_mbc(struct GB_Cart* mbc, const struct GB_CartHeader* header) {
	memset(MBC_NO_RAM, 0xFF, sizeof(MBC_NO_RAM));

	if (!mbc || !header) {
		return false;
	}

	// this won't fail because the type is 8-bit and theres 0x100 entries.
	// though the data inside can be NULL, but this is checked next...
	const struct GB_MbcInterface* data = &MBC_SETUP_DATA[header->cart_type];
	
	// this is a check to see if we have data.
	// i could check only 1 func, but just in case i check all 3.
	if (!data->write || !data->get_rom_bank || !data->get_ram_bank) {
		printf("MBC NOT IMPLEMENTED: 0x%02X\n", header->cart_type);
		assert(0);
		return false;
	}

	// set the function ptr's and flags!
	mbc->write = data->write;
	mbc->get_rom_bank = data->get_rom_bank;
	mbc->get_ram_bank = data->get_ram_bank;
	mbc->rom_bank = data->starting_rom_bank;
	mbc->rom_bank_lo = data->starting_rom_bank;
	mbc->flags = data->flags;

	// setup max rom banks
	// this can never be 0 as rom size is already set before.
	assert(mbc->rom_size > 0 && "you changed where rom size is set!");
	mbc->rom_bank_max = mbc->rom_size / 0x4000;

	// todo: setup more flags.
	if (mbc->flags & MBC_FLAGS_RAM) {
		// check if mbc2, if so, manually set the ram size!
		if (header->cart_type == 0x5 || header->cart_type == 0x6) {
			mbc->ram_size = 0x200;
		}
		// otherwise get the ram size via a LUT
		else if (!GB_get_cart_ram_size(header->ram_size, &mbc->ram_size)) {
			printf("rom has ram but the size entry in header is invalid! %u\n", header->ram_size);
			return false;
		}
		else {
			mbc->ram_bank_max = mbc->ram_size / 0x2000;
		}
	}

	return true;
}
