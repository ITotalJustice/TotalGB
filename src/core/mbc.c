#include "gb.h"
#include "internal.h"
#include "mbc/mbc_0.h"
#include "mbc/mbc_1.h"
#include "mbc/mbc_3.h"
#include "mbc/mbc_5.h"

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
const GB_U8 MBC_NO_RAM[0x2000];

struct GB_MbcSetupData {
	void (*write)(struct GB_Data *gb, GB_U16 addr, GB_U8 value);
	const GB_U8* (*get_rom_bank)(struct GB_Data *gb, GB_U8 bank);
	const GB_U8* (*get_ram_bank)(struct GB_Data *gb, GB_U8 bank);
	GB_U16 starting_rom_bank;
	GB_U8 flags;
};

// this data is indexed using the [cart_type] member from the
// cart header struct.
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

int GB_get_rom_name(const struct GB_Data* gb, struct GB_CartName* name) {
	assert(gb && name);

	const struct GB_CartHeader* header = GB_get_rom_header_ptr(gb);
	assert(header);

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
		// not upper-case ascii
		if ((c < 'A' || c > 'Z') && c != ' ') {
			break;
		}
		name->name[i] = c;
	}

	return 0;
}

GB_BOOL GB_get_cart_ram_size(GB_U8 type, GB_U32* size) {
	// i think that more ram sizes are valid, however
	// i have yet to see a ram size bigger than this...
	static const GB_U32 GB_CART_RAM_SIZES[6] = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
	
	assert(type < GB_ARR_SIZE(GB_CART_RAM_SIZES) && "OOB type access!");

	if (type >= GB_ARR_SIZE(GB_CART_RAM_SIZES)) {
		return GB_FALSE;
	}

	*size = GB_CART_RAM_SIZES[type];
	return GB_TRUE;
}

GB_BOOL GB_setup_mbc(struct GB_Cart* mbc, const struct GB_CartHeader* header) {
	if (!mbc || !header) {
		return GB_FALSE;
	}

	// this won't fail because the type is 8-bit and theres 0x100 entries.
	// though the data inside can be NULL, but this is checked next...
	const struct GB_MbcSetupData* data = &MBC_SETUP_DATA[header->cart_type];
	
	// this is a check to see if we have data.
	// i could check only 1 func, but just in case i check all 3.
	if (!data->write || !data->get_rom_bank || !data->get_ram_bank) {
		printf("MBC NOT IMPLEMENTED: 0x%02X\n", header->cart_type);
		assert(0);
		return GB_FALSE;
	}

	// set the function ptr's and flags!
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
