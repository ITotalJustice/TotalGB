#include "core/gb.h"
#include "core/internal.h"

#include <stdio.h>
#include <assert.h>


static inline void GB_iowrite_gbc(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	assert(GB_is_system_gbc(gb) == true);

	switch (addr & 0x7F) {
		case 0x4D:
			IO_KEY1 |= value & 0x1;
			printf("writing to key1 0x%02X\n", value);
			break;

		case 0x4F: // (VBK)
			IO_VBK = value & 1;
			GB_update_vram_banks(gb);
			break;

		case 0x50: // unused (bootrom?)
			break;

		case 0x51: // (HDMA1)
			IO_HDMA1 = value;
			break;

		case 0x52: // (HDMA2)
			IO_HDMA2 = value;
			break;

		case 0x53: // (HMDA3)
			IO_HDMA3 = value;
			break;

		case 0x54: // (HDMA4)
			IO_HDMA4 = value;
			break;

		case 0x55: // (HDMA5)
			GB_hdma5_write(gb, value);
			break;

		case 0x68: // BCPS
			IO_BCPS = value;
			break;

		case 0x69: // BCPD
			GB_bcpd_write(gb, value);
			break;

		case 0x6A: // OCPS
			IO_OCPS = value;
			break;

		case 0x6B: // OCPD
			GB_ocpd_write(gb, value);
			break;

		case 0x6C: // OPRI
			assert(0&&"opri");
			IO_OPRI = value & 1;
			printf("[INFO] IO_OPRI %u\n", value & 1);
			break;

		case 0x70: // (SVBK) always set between 1-7
			IO_SVBK = (value & 0x07) + ((value & 0x07) == 0x00);
			GB_update_wram_banks(gb);
			break;

		case 0x72:
			IO_72 = value;
			break;

		case 0x73:
			IO_73 = value;
			break;

		case 0x74:
			IO_74 = value;
			break;

		case 0x75: // only bits 4-6 are useable
			IO_75 = value & 0x70;
			break;
	}
}

uint8_t GB_ioread(const struct GB_Core* gb, uint16_t addr) {
	switch (addr & 0x7F) {
		case 0x00:
			return GB_joypad_read(gb);

		case 0x01:
			return 0xFF;
			// return GB_serial_sb_read(gb);

		case 0x02: // SC
			return 0x7C; // all zeros apart from unused bits

		case 0x03:
			return 0xFF;

		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1A: case 0x1B:
		case 0x1C: case 0x1D: case 0x1E: case 0x20:
		case 0x21: case 0x22: case 0x23: case 0x24:
		case 0x25: case 0x26:
		case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
			return GB_apu_ioread(gb, addr);

		case 0x27: case 0x28: case 0x29: case 0x2A:
		case 0x2B: case 0x2C: case 0x2D: case 0x2E:
		case 0x2F: case 0x1F: // unused
			return 0xFF;

		case 0x4D:
			if (GB_is_system_gbc(gb) == true) {
				printf("reading key1 0x%02X\n", IO_KEY1);
				return 0x7E | IO_KEY1;
			} else {
				return 0xFF;
			}

		case 0x4F: // (GBC) VBK
			if (GB_is_system_gbc(gb) == true) {
				return IO_VBK;
			} else {
				return 0xFF;
			}
			

		case 0x55: // (GBC) HDMA5
			if (GB_is_system_gbc(gb) == true) {
				return GB_hdma5_read(gb);
			} else {
				return 0xFF;
			}

		case 0x56:
			if (GB_is_system_gbc(gb) == true) {
				printf("reading from infrared port\n");
				return 0x00;
			} else {
				return 0xFF;
			}

		case 0x69:
			if (GB_is_system_gbc(gb) == true) {
				return GBC_bcpd_read(gb);
			} else {
				return 0xFF;
			}

		case 0x6B:
			if (GB_is_system_gbc(gb) == true) {
				return GBC_ocpd_read(gb);
			} else {
				return 0xFF;
			}

		case 0x70: // (GBC) SVBK
			if (GB_is_system_gbc(gb) == true) {
				return IO_SVBK;
			} else {
				return 0xFF;
			}

		default:
			return IO[addr & 0x7F];
	}
}

void GB_iowrite(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	switch (addr & 0x7F) {
		case 0x00: // joypad
			GB_joypad_write(gb, value);
			break;

		case 0x01: // SB (Serial transfer data)
			IO_SB = value;
			break;

		case 0x02: // SC (Serial Transfer Control)
			GB_serial_sc_write(gb, value);
			break;

		case 0x03: // IO_DIV lower, non writeable
			break;

		case 0x04:
			IO_DIV_UPPER = IO_DIV_LOWER = 0;
			break;

		case 0x05:
			IO_TIMA = value;
			break;

		case 0x06:
			IO_TMA = value;
			break;

		case 0x07:
			IO_TAC = (value | 248);
			break;
		
		case 0x0F:
			IO_IF = (value | 224);
			break;

		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x16: case 0x17: case 0x18:
		case 0x19: case 0x1A: case 0x1B: case 0x1C:
		case 0x1D: case 0x1E: case 0x20: case 0x21:
		case 0x22: case 0x23: case 0x24: case 0x25:
		case 0x26:
		case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
			GB_apu_iowrite(gb, addr, value);
			break;

		case 0x40:
			GB_on_lcdc_write(gb, value);
			break;

		case 0x41:
			IO_STAT = (IO_STAT & 0x7) | (value & 0x78) | 0x80;
			GB_compare_LYC(gb);
			break;

		case 0x42:
			IO_SCY = value;
			break;

		case 0x43:
			IO_SCX = value;
			break;

		case 0x45:
			IO_LYC = value;
			GB_compare_LYC(gb);
			break;

		case 0x46:
			IO_DMA = value;
			GB_DMA(gb);
			break;

		case 0x47:
			gb->ppu.dirty_bg[0] |= (IO_BGP != value);
			IO_BGP = value;
			break;

		case 0x48:
			gb->ppu.dirty_obj[0] |= (IO_OBP0 != value);
			IO_OBP0 = value;
			break;

		case 0x49:
			gb->ppu.dirty_obj[1] |= (IO_OBP1 != value);
			IO_OBP1 = value;
			break;

		case 0x4A:
			IO_WY = value;
			break;

		case 0x4B:
			IO_WX = value;
			break;

		default:
			if (GB_is_system_gbc(gb) == true) {
				GB_iowrite_gbc(gb, addr, value);
			}
			break;
	}
}

uint8_t GB_read8(const struct GB_Core* gb, const uint16_t addr) {
	// fast routine!
    if (addr <= 0xBFFF) {
        return gb->map[addr >> 13][addr & 0x1FFF];//gb->mask[addr >> 13]];
    }

    // slow
    else if (addr >= 0xC000 && addr <= 0xCFFF) {
    	return gb->wram[0][addr & 0x0FFF];
    }
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
    	return gb->wram[gb->wram_bank][addr & 0x0FFF];
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return gb->ppu.oam[addr & 0xFF];
    }
	else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return GB_ioread(gb, addr);
    }
	else if (addr >= 0xFF80) {
        return gb->hram[addr & 0x7F];
    }

    // this is echo ram area, almost never used, Nintendo
    // even outlined this area as banned in the programming manual.
    // however, this area is technically still valid, so, this area is
    // added to the least common if else branch
    else if (addr >= 0xE000 && addr <= 0xEFFF) {
    	return gb->wram[0][addr & 0x0FFF];
    }
    else if (addr >= 0xF000 && addr <= 0xFDFF) {
    	return gb->wram[gb->wram_bank][addr & 0x0FFF];
    }

	// unused section in address area.
	return 0xFF;
}

void GB_write8(struct GB_Core* gb, uint16_t addr, uint8_t value) {
	if (LIKELY(addr < 0xFE00)) {
		switch ((addr >> 12) & 0xF) {
			case 0x0: case 0x1: case 0x2: case 0x3: case 0x4:
			case 0x5: case 0x6: case 0x7: case 0xA: case 0xB:
				gb->cart.write(gb, addr, value);
				break;
			
			case 0x8: case 0x9:
				gb->ppu.vram[gb->vram_bank][addr & 0x1FFF] = value;
				break;
			
			case 0xC: case 0xE:
				gb->wram[0][addr & 0x0FFF] = value;
				break;
			
			case 0xD: case 0xF:
				gb->wram[gb->wram_bank][addr & 0x0FFF] = value;
				break;
		}
	}
	else if (addr <= 0xFE9F) {
        gb->ppu.oam[addr & 0xFF] = value;
    }
	else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        GB_iowrite(gb, addr, value);
    }
	else if (addr >= 0xFF80) {
        gb->hram[addr & 0x7F] = value;
    }
}

uint16_t GB_read16(const struct GB_Core* gb, uint16_t addr) {
	const uint8_t lo = GB_read8(gb, addr + 0);
	const uint8_t hi = GB_read8(gb, addr + 1);

	return (hi << 8) | lo;
}

void GB_write16(struct GB_Core* gb, uint16_t addr, uint16_t value) {
	GB_write8(gb, addr + 0, value & 0xFF);
    GB_write8(gb, addr + 1, value >> 8);
}

void GB_update_rom_banks(struct GB_Core* gb) {
	const struct MBC_BankInfo rom_bank0 = gb->cart.get_rom_bank(gb, 0);
	const struct MBC_BankInfo rom_bankx = gb->cart.get_rom_bank(gb, 1);

	gb->map[0x0] = rom_bank0.ptr + 0x0000;
	gb->map[0x1] = rom_bank0.ptr + 0x2000;
	gb->map[0x2] = rom_bankx.ptr + 0x0000;
	gb->map[0x3] = rom_bankx.ptr + 0x2000;

	gb->mask[0x0] = rom_bank0.mask;
	gb->mask[0x1] = rom_bank0.mask;
	gb->mask[0x2] = rom_bankx.mask;
	gb->mask[0x3] = rom_bankx.mask;
}

void GB_update_ram_banks(struct GB_Core* gb) {
	const struct MBC_BankInfo info = gb->cart.get_ram_bank(gb);

	gb->map[0x5] = info.ptr;
	gb->mask[0x5] = info.mask;
}

void GB_update_vram_banks(struct GB_Core* gb) {
	gb->mask[0x4] = 0x1FFF;
	gb->vram_bank = 0;

	if (GB_is_system_gbc(gb) == true) {
		gb->map[0x4] = gb->ppu.vram[IO_VBK];
		gb->vram_bank = IO_VBK;
	}
	else {
		gb->map[0x4] = gb->ppu.vram[0];
		gb->vram_bank = 0;
	}
}

void GB_update_wram_banks(struct GB_Core* gb) {
	if (GB_is_system_gbc(gb) == true) {
		gb->wram_bank = 1;
		gb->wram_bank = IO_SVBK;
	}
	else {
		// fixed to 1 on non-GBC mode
		gb->wram_bank = 1;
	}
}

void GB_setup_mmap(struct GB_Core* gb) {
	GB_update_rom_banks(gb);
	GB_update_ram_banks(gb);
	GB_update_vram_banks(gb);
	GB_update_wram_banks(gb);
}
