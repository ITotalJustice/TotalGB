#include "core/mbc/common.h"


void GB_mbc2_write(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	switch ((addr >> 12) & 0xF) {
	// RAM ENABLE / ROM BANK
        case 0x0: case 0x1: case 0x2: case 0x3:
            // if the 8th bit is not set, the value
            // controls the ram enable
            if ((addr & 0x100) == 0) {
                gb->cart.ram_enabled = (value & 0x0F) == 0x0A;
                GB_update_ram_banks(gb);
            } else {
                const uint8_t bank = (value & 0x0F) | ((value & 0x0F) == 0);
                gb->cart.rom_bank = bank % gb->cart.rom_bank_max;
                GB_update_rom_banks(gb);
            }
            break;

    // RAM WRITE
        case 0xA: case 0xB: {
            if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
                return;
            }
            // because of mbc2 being only 512-bytes and due to my mmap
            // pointer system, we must manually mirror all writes to
            // all addresses in range 0x0000 - 0x1FFFF.
            const uint8_t masked_value = (value & 0x0F) | 0xF0;
            const uint16_t masked_addr = addr & 0x1FF;

            gb->cart.ram[0x0000 + masked_addr] = masked_value;
            gb->cart.ram[0x0200 + masked_addr] = masked_value;
            gb->cart.ram[0x0400 + masked_addr] = masked_value;
            gb->cart.ram[0x0600 + masked_addr] = masked_value;
            gb->cart.ram[0x0800 + masked_addr] = masked_value;
            gb->cart.ram[0x0A00 + masked_addr] = masked_value;
            gb->cart.ram[0x0C00 + masked_addr] = masked_value;
            gb->cart.ram[0x0E00 + masked_addr] = masked_value;
            gb->cart.ram[0x1000 + masked_addr] = masked_value;
            gb->cart.ram[0x1200 + masked_addr] = masked_value;
            gb->cart.ram[0x1400 + masked_addr] = masked_value;
            gb->cart.ram[0x1600 + masked_addr] = masked_value;
            gb->cart.ram[0x1800 + masked_addr] = masked_value;
            gb->cart.ram[0x1A00 + masked_addr] = masked_value;
            gb->cart.ram[0x1C00 + masked_addr] = masked_value;
            gb->cart.ram[0x1E00 + masked_addr] = masked_value;
        } break;
	}
}

const uint8_t* GB_mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
	if (bank == 0) {
		return gb->cart.rom;
	}
	
	return gb->cart.rom + (gb->cart.rom_bank * 0x4000);
}

const uint8_t* GB_mbc2_get_ram_bank(struct GB_Core* gb, uint8_t bank) {
	GB_UNUSED(bank);
    
	if (!(gb->cart.flags & MBC_FLAGS_RAM) || !gb->cart.ram_enabled) {
		return MBC_NO_RAM;
	}

	return gb->cart.ram;
}
