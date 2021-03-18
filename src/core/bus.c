#include "gb.h"
#include "internal.h"

#if defined(GB_DEBUG) || defined(GB_LOG)
#include <stdio.h>
#endif

#include <assert.h>

#define GB_is_sound_enabled() (!!(IO_NR52 & 0x80))

GB_U8 GB_ioread(struct GB_Data* gb, GB_U16 addr) {
	switch (addr & 0x7F) {
	case 0x00:
		return GB_joypad_get(gb); // TODO: return joybuffer
	case 0x01: // serial
		return 0xFF;
	case 0x02: // serial
		return 0xFF;
	case 0x4D: // double speed
		return 0xFF;
	case 0x70: // SVBK
		return 0xFF;
	default:
		return IO[addr & 0x7F];
	}
}

#define GB_disable_sound_regs() do { \
	IO_NR10 = 0xFF; \
	IO_NR11 = 0xFF; \
	IO_NR12 = 0xFF; \
	IO_NR13 = 0xFF; \
	IO_NR14 = 0xFF; \
	IO_NR21 = 0xFF; \
	IO_NR22 = 0xFF; \
	IO_NR23 = 0xFF; \
	IO_NR24 = 0xFF; \
	IO_NR30 = 0xFF; \
	IO_NR31 = 0xFF; \
	IO_NR32 = 0xFF; \
	IO_NR33 = 0xFF; \
	IO_NR34 = 0xFF; \
	IO_NR41 = 0xFF; \
	IO_NR42 = 0xFF; \
	IO_NR43 = 0xFF; \
	IO_NR44 = 0xFF; \
	IO_NR50 = 0xFF; \
	IO_NR51 = 0xFF; \
} while(0)

void GB_iowrite(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	switch (addr & 0x7F) {
	case 0x01: // serial
		// putchar(value);
		break;
	case 0x02: // serial
		// putchar(value);
		break;
	case 0x03: // unused
		break;
	case 0x04:
		IO_DIV = 0;
		break;
	case 0x07:
		IO_TAC = (value | 248);
		break;
	case 0x08: // unused
		break;
	case 0x09: // unused
		break;
	case 0x0A: // unused
		break;
	case 0x0B: // unused
		break;
	case 0x0C: // unused
		break;
	case 0x0D: // unused
		break;
	case 0x0E: // unused
		break;
	case 0x0F:
		IO_IF = (value | 224);
		break;
	case 0x10:
		IO_NR10 = (0xFF * !(GB_is_sound_enabled())) | ((value | 0x80) * GB_is_sound_enabled());
		break;
	case 0x11:
		IO_NR11 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x12:
		IO_NR12 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x13:
		IO_NR13 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x14:
		IO_NR14 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x15: // unused
		break;
	case 0x16:
		IO_NR21 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x17:
		IO_NR22 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x18:
		IO_NR23 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x19:
		IO_NR24 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x1A:
		IO_NR30 = (0xFF * !(GB_is_sound_enabled())) | ((value | 0x7F) * GB_is_sound_enabled());
		break;
	case 0x1B:
		IO_NR31 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x1C:
		IO_NR32 = (0xFF * !(GB_is_sound_enabled())) | ((value | 159) * GB_is_sound_enabled());
		break;
	case 0x1D:
		IO_NR33 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x1E:
		IO_NR34 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x1F: // unused
		break;
	case 0x20:
		IO_NR41 = (0xFF * !(GB_is_sound_enabled())) | ((value | 0xC0) * GB_is_sound_enabled());
		break;
	case 0x21:
		IO_NR42 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x22:
		IO_NR43 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x23:
		IO_NR44 = (0xFF * !(GB_is_sound_enabled())) | ((value | 63) * GB_is_sound_enabled());
		break;
	case 0x24:
		IO_NR50 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x25:
		IO_NR51 = (0xFF * !(GB_is_sound_enabled())) | (value * GB_is_sound_enabled());
		break;
	case 0x26:
		IO_NR52 = ((IO_NR52 & 0xF) | (value & 0x80));
		if (!GB_is_sound_enabled()) {
			GB_disable_sound_regs();
		}
		// IO_NR52 = ((value & 0x7) | 0x70);
		// if (!GB_is_sound_enabled()) {
		// 	GB_disable_sound_regs();
		// }
		break;
	case 0x27: // unused
		break;
	case 0x28: // unused
		break;
	case 0x29: // unused
		break;
	case 0x40:
		if ((IO_LCDC & 0x80) && (!(value & 0x80))) {
			IO_LY = 0;
			IO_STAT &= 0xFC;
			GB_set_status_mode(gb, 0);
		} else if ((!(IO_LCDC & 0x80)) && (value & 0x80)) {
			IO_LY = 0;
			gb->ppu.line_counter = 0;
			GB_compare_LYC(gb);
			GB_set_status_mode(gb, 2);
		}
		IO_LCDC = value;
		break;
	case 0x41:
		IO_STAT = (IO_STAT & 0x7) | (value & (120)) | 0x80;
		break;
	case 0x44: // LY is read only (i think)
		break;
	case 0x45:
		IO_LYC = value;
		if (IO_LCDC & 0x80) {
			GB_compare_LYC(gb);
		}
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
	case 0x4C: // unused
		break;
	case 0x4E: // unused
		break;
	case 0x4D: // unused (KEY1)
		break;
	case 0x4F: // unused (VBK)
		break;
	case 0x50: // unused (bootrom?)
		break;
	case 0x51: // unused (HDMA1)
		break;
	case 0x52: // unused (HDMA2)
		break;
	case 0x53: // unused (HMDA3)
		break;
	case 0x54: // unused (HDMA4)
		break;
	case 0x55: // unused (HDMA5)
		break;
	case 0x56: // unused
		break;
	case 0x57: // unused
		break;
	case 0x58: // unused
		break;
	case 0x59: // unused
		break;
	case 0x5A: // unused
		break;
	case 0x5B: // unused
		break;
	case 0x5C: // unused
		break;
	case 0x5D: // unused
		break;
	case 0x5E: // unused
		break;
	case 0x5F: // unused
		break;
	case 0x60: // unused
		break;
	case 0x61: // unused
		break;
	case 0x62: // unused
		break;
	case 0x63: // unused
		break;
	case 0x64: // unused
		break;
	case 0x65: // unused
		break;
	case 0x66: // unused
		break;
	case 0x67: // unused
		break;
	case 0x68: // unused
		break;
	case 0x69: // unused
		break;
	case 0x6A: // unused
		break;
	case 0x6B: // unused
		break;
	case 0x6C: // unused
		break;
	case 0x6D: // unused
		break;
	case 0x6E: // unused
		break;
	case 0x6F: // unused
		break;
	case 0x70: // unused (SVBK) (doesnt work yet as i set to 0)
		break;
	case 0x71: // unused
		break;
	case 0x72: // unused
		break;
	case 0x73: // unused
		break;
	case 0x74: // unused
		break;
	case 0x75: // unused
		break;
	case 0x76: // unused
		break;
	case 0x77: // unused
		break;
	case 0x78: // unused
		break;
	case 0x79: // unused
		break;
	case 0x7A: // unused
		break;
	case 0x7B: // unused
		break;
	case 0x7C: // unused
		break;
	case 0x7D: // unused
		break;
	case 0x7E: // unused
		break;
	case 0x7F: // unused
		break;
	default:
		IO[addr & 0x7F] = value;
		break;
	}
}

GB_U8 GB_read8(struct GB_Data* gb, const GB_U16 addr) {
	if (LIKELY(addr < 0xFE00)) {
		return gb->mmap[(addr >> 12)][addr & 0x0FFF];
	} else if (addr <= 0xFE9F) {
        return gb->ppu.oam[addr & 0x9F];
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return GB_ioread(gb, addr);
    } else if (addr >= 0xFF80) {
        return gb->hram[addr & 0x7F];
    }

	// unused section in address area.
	return 0xFF;
}

void GB_write8(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	if (LIKELY(addr < 0xFE00)) {
		switch ((addr >> 12) & 0xF) {
		case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
			gb->cart.mbc.write(gb, addr, value);
			break;
		case 0x8: case 0x9:
			gb->ppu.vram[0][addr & 0x1FFF] = value;
			break;
		case 0xA: case 0xB:
			gb->cart.mbc.write(gb, addr, value);
			break;
		case 0xC:
			gb->wram[0][addr & 0x0FFF] = value;
			break;
		case 0xD:
			gb->wram[1][addr & 0x0FFF] = value;
			break;
		case 0xE:
			gb->wram[0][addr & 0x0FFF] = value;
			break;
		case 0xF:
			gb->wram[1][addr & 0x0FFF] = value;
			break;
		default: __builtin_unreachable();
		}
	} else if (addr <= 0xFE9F) {
        gb->ppu.oam[addr & 0x9F] = value;
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        GB_iowrite(gb, addr, value);
    } else if (addr >= 0xFF80) {
        gb->hram[addr & 0x7F] = value;
    }
}

GB_U16 GB_read16(struct GB_Data* gb, GB_U16 addr) {
	return (GB_read8(gb, addr) | (GB_read8(gb, addr + 1) << 8));
}

void GB_write16(struct GB_Data* gb, GB_U16 addr, GB_U16 value) {
	GB_write8(gb, addr, (value & 0xFF));
    GB_write8(gb, addr + 1, (value >> 8) & 0xFF);
}

void GB_update_rom_banks(struct GB_Data* gb) {
	const GB_U8* rom_bank0 = gb->cart.mbc.get_rom_bank(gb, 0);
	const GB_U8* rom_bankx = gb->cart.mbc.get_rom_bank(gb, 1);
	gb->mmap[0x0] = rom_bank0 + 0x0000;
	gb->mmap[0x1] = rom_bank0 + 0x1000;
	gb->mmap[0x2] = rom_bank0 + 0x2000;
	gb->mmap[0x3] = rom_bank0 + 0x3000;

	gb->mmap[0x4] = rom_bankx + 0x0000;
	gb->mmap[0x5] = rom_bankx + 0x1000;
	gb->mmap[0x6] = rom_bankx + 0x2000;
	gb->mmap[0x7] = rom_bankx + 0x3000;
}

void GB_update_ram_banks(struct GB_Data* gb) {
	const GB_U8* cart_ram = gb->cart.mbc.get_ram_bank(gb, 0);
	gb->mmap[0xA] = cart_ram + 0x0000;
	gb->mmap[0xB] = cart_ram + 0x1000;
}

void GB_update_vram_banks(struct GB_Data* gb) {
	gb->mmap[0x8] = gb->ppu.vram[IO_VBK] + 0x0000;
	gb->mmap[0x9] = gb->ppu.vram[IO_VBK] + 0x1000;
}

void GB_update_wram_banks(struct GB_Data* gb) {
	gb->mmap[0xD] = gb->wram[IO_SVBK];
	gb->mmap[0xF] = gb->wram[IO_SVBK];
}

void GB_setup_mmap(struct GB_Data* gb) {
	GB_update_rom_banks(gb);
	GB_update_ram_banks(gb);
	gb->mmap[0x8] = gb->ppu.vram[0] + 0x0000;
	gb->mmap[0x9] = gb->ppu.vram[0] + 0x1000;
	gb->mmap[0xC] = gb->wram[0];
	gb->mmap[0xD] = gb->wram[1];
	gb->mmap[0xE] = gb->wram[0];
	gb->mmap[0xF] = gb->wram[1];
}
