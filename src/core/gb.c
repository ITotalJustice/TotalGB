#include "gb.h"
#include "tables/palette_table.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ROM_SIZE_MULT 0x8000

#if !defined GB_NO_STDLIB
#include <stdlib.h>
#endif

GB_BOOL GB_init(struct GB_Data* gb) {
	if (!gb) {
		return GB_FALSE;
	}

	gb->cart.mbc_type = MBC_TYPE_NONE;
	GB_Palette_fill_from_custom(GB_CUSTOM_PALETTE_CREAM, &gb->palette);

	// set all the callbacks and data to NULL
	GB_set_vblank_callback(gb, NULL, NULL);
	GB_set_hblank_callback(gb, NULL, NULL);
	GB_set_dma_callback(gb, NULL, NULL);
	GB_set_halt_callback(gb, NULL, NULL);
	GB_set_stop_callback(gb, NULL, NULL);
	GB_set_error_callback(gb, NULL, NULL);
	GB_connect_link_cable(gb, NULL, NULL);

	gb->is_master = GB_FALSE;

	return GB_TRUE;
}

void GB_quit(struct GB_Data* gb) {
	assert(gb);

	#define FREE_ELSE_WARN(name, func) \
	if (gb->cart.name != NULL) { \
		if (func != NULL) { \
			func(gb->cart.name); \
			gb->cart.name = NULL; \
		} else { \
			printf("[GB-WARN] Leaking %s\n", #name); \
		} \
	}

	FREE_ELSE_WARN(rom, gb->rom_free_func);
}

void GB_reset(struct GB_Data* gb) {
	memset(gb->wram, 0, sizeof(gb->wram));
	memset(gb->hram, 0, sizeof(gb->hram));
	memset(IO, 0xFF, sizeof(IO));
	memset(gb->ppu.vram, 0, sizeof(gb->ppu.vram));
	memset(gb->ppu.bg_palette, 0, sizeof(gb->ppu.bg_palette));
	memset(gb->ppu.obj_palette, 0, sizeof(gb->ppu.obj_palette));
	memset(gb->ppu.pixles, 0, sizeof(gb->ppu.pixles));
	memset(gb->ppu.bg_colours, 0, sizeof(gb->ppu.bg_colours));
	memset(gb->ppu.obj_colours, 0, sizeof(gb->ppu.obj_colours));
	memset(gb->ppu.dirty_bg, 0, sizeof(gb->ppu.dirty_bg));
	memset(gb->ppu.dirty_obj, 0, sizeof(gb->ppu.dirty_obj));
	GB_update_all_colours_gb(gb);
	gb->joypad.var = 0xFF;
	gb->ppu.next_cycles = 0;
	gb->timer.next_cycles = 0;
	gb->ppu.line_counter = 0;
	gb->cpu.cycles = 0;
	gb->cpu.halt = 0;
	gb->cpu.ime = 0;
	// CPU
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_AF, 0x01B0);
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_BC, 0x0013);
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_DE, 0x00D8);
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_HL, 0x014D);
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_SP, 0xFFFE);
	GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_PC, 0x0100);
	// IO
	IO_TIMA = 0x00;
	IO_TMA = 0x00;
	IO_TAC = 0x00;
	IO_NR10 = 0x80;
	IO_NR11 = 0xBF;
	IO_NR12 = 0xF3;
	IO_NR14 = 0xBF;
	IO_NR21 = 0x3F;
	IO_NR22 = 0x00;
	IO_NR24 = 0xBF;
	IO_NR30 = 0x7F;
	IO_NR31 = 0xFF;
	IO_NR32 = 0x9F;
	IO_NR34 = 0xBF;
	IO_NR41 = 0xFF;
	IO_NR42 = 0x00;
	IO_NR43 = 0x00;
	IO_NR44 = 0xBF;
	IO_NR50 = 0x77;
	IO_NR51 = 0xF3;
	IO_NR52 = 0xF1;
	IO_SVBK = 0x01;
	IO_LCDC = 0x91;
	IO_STAT = 0x00;
	IO_SCY = 0x00;
	IO_SCX = 0x00;
	IO_LY = 0x00;
	IO_LYC = 0x00;
	IO_BGP = 0xFC;
	IO_OBP0 = 0xFF;
	IO_OBP1 = 0xFF;
	IO_WY = 0x00;
	IO_WX = 0x00;
	IO_IE = 0x00;
}

static const char* cart_type_str(const GB_U8 type) {
    switch (type) {
        case 0x00:  return "ROM ONLY";                 case 0x19:  return "MBC5";
        case 0x01:  return "MBC1";                     case 0x1A:  return "MBC5+RAM";
        case 0x02:  return "MBC1+RAM";                 case 0x1B:  return "MBC5+RAM+BATTERY";
        case 0x03:  return "MBC1+RAM+BATTERY";         case 0x1C:  return "MBC5+RUMBLE";
        case 0x05:  return "MBC2";                     case 0x1D:  return "MBC5+RUMBLE+RAM";
        case 0x06:  return "MBC2+BATTERY";             case 0x1E:  return "MBC5+RUMBLE+RAM+BATTERY";
        case 0x08:  return "ROM+RAM";                  case 0x20:  return "MBC6";
        case 0x09:  return "ROM+RAM+BATTERY";          case 0x22:  return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
        case 0x0B:  return "MMM01";
        case 0x0C:  return "MMM01+RAM";
        case 0x0D:  return "MMM01+RAM+BATTERY";
        case 0x0F:  return "MBC3+TIMER+BATTERY";
        case 0x10:  return "MBC3+TIMER+RAM+BATTERY";   case 0xFC:  return "POCKET CAMERA";
        case 0x11:  return "MBC3";                     case 0xFD:  return "BANDAI TAMA5";
        case 0x12:  return "MBC3+RAM";                 case 0xFE:  return "HuC3";
        case 0x13:  return "MBC3+RAM+BATTERY";         case 0xFF:  return "HuC1+RAM+BATTERY";
        default: return "NULL";
    }
}

static void cart_header_print(const struct GB_CartHeader* header) {
    printf("\nROM HEADER INFO\n");
    printf("TITLE: ");
    for (int i = 0; i < 0x10 && header->title[i] >= 32 && header->title[i] < 127; i++) {
        putchar(header->title[i]);
    }
    putchar('\n');
    printf("NEW LICENSEE CODE: 0x%02X\n", header->new_licensee_code);
    printf("SGB FLAG: 0x%02X\n", header->sgb_flag);
    printf("CART TYPE: %s\n", cart_type_str(header->cart_type));
    printf("CART TYPE VALUE: 0x%02X\n", header->cart_type);
    printf("ROM SIZE: 0x%02X\n", header->rom_size);
    printf("RAM SIZE: 0x%02X\n", header->ram_size);
	printf("HEADER CHECKSUM: 0x%02X\n", header->header_checksum);
	printf("GLOBAL CHECKSUM: 0x%04X\n", header->global_checksum);
	GB_U8 hash, forth;
	GB_get_rom_palette_hash_from_header(header, &hash, &forth);
	printf("HASH: 0x%02X, 0x%02X\n", hash, forth);
    putchar('\n');
}

static const struct GB_CartHeader* GB_get_rom_header_from_data(const GB_U8* data) {
	assert(data);
	return (struct GB_CartHeader*)&data[GB_BOOTROM_SIZE];
}

const struct GB_CartHeader* GB_get_rom_header(const struct GB_Data* gb) {
	assert(gb && gb->cart.rom);
	return GB_get_rom_header_from_data(gb->cart.rom);
}

GB_BOOL GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, GB_U8* hash, GB_U8* forth) {
	assert(header && hash && forth);

	if (!header || !hash || !forth) {
		return GB_FALSE;
	}

	GB_U16 temp_hash = 0;
	for (GB_U16 i = 0; i < sizeof(header->title); ++i) {
		temp_hash += header->title[i];
	}

	*hash = temp_hash & 0xFF;
	*forth = header->title[0x3];

	return GB_TRUE;
}

GB_BOOL GB_get_rom_palette_hash(const struct GB_Data* gb, GB_U8* hash, GB_U8* forth) {
	assert(gb && hash);

	if (!gb || !hash) {
		return GB_FALSE;
	}

	return GB_get_rom_palette_hash_from_header(
		GB_get_rom_header(gb),
		hash, forth
	);
}

GB_BOOL GB_set_palette_from_palette(struct GB_Data* gb, const struct GB_PaletteEntry* palette) {
	assert(gb && palette);

	if (!gb || !palette) {
		return GB_FALSE;
	}

	gb->palette = *palette;

	return GB_TRUE;
}

// these 3 funcs can be simplified by making a mem IFile struct
// an passing it to loadrom_ifile();
int GB_loadrom_data(struct GB_Data* gb, GB_U8* data, GB_U32 size, void(*free_func)(void*)) {
	if (!gb || !data || !size || size < GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader)) {
		return -1;
	}

	const struct GB_CartHeader* header = GB_get_rom_header_from_data(data);
	cart_header_print(header);

	const GB_U32 rom_size = ROM_SIZE_MULT << header->rom_size;
	if (rom_size > size) {
		return -1;
	}

	gb->cart.rom_size = rom_size;
	gb->cart.rom = data;

	if (!GB_setup_mbc(&gb->cart, header)) {
		return -1;
	}

	GB_reset(gb);
	GB_setup_mmap(gb);

	GB_U8 hash, forth;
	if (GB_get_rom_palette_hash_from_header(header, &hash, &forth)) {
		struct GB_PaletteEntry palette;
		
		if (0 == GB_palette_fill_from_hash(hash, forth, &palette)) {
			GB_set_palette_from_palette(gb, &palette);
		}
	}

	gb->rom_free_func = free_func;

	return 0;
}

GB_BOOL GB_has_save(const struct GB_Data* gb) {
	assert(gb);
	return (gb->cart.flags & (MBC_FLAGS_RAM | MBC_FLAGS_BATTERY)) > 0;
}

GB_BOOL GB_has_rtc(const struct GB_Data* gb) {
	assert(gb);
	return (gb->cart.flags & MBC_FLAGS_RTC) > 0;
}

GB_U32 GB_calculate_savedata_size(const struct GB_Data* gb) {
	assert(gb);
	return gb->cart.ram_size;
}

int GB_savegame(const struct GB_Data* gb, struct GB_SaveData* save) {
	assert(gb && save);

	if (!GB_has_save(gb)) {
		printf("[GB-ERROR] trying to savegame when cart doesn't support battery ram!\n");
		return -1;
	}

	/* larger sizes would techinally be fine... */
	save->size = GB_calculate_savedata_size(gb);
	memcpy(save->data, gb->cart.ram, save->size);

	if (GB_has_rtc(gb) == GB_TRUE) {
		memcpy(&save->rtc, &gb->cart.rtc, sizeof(save->rtc));
		save->has_rtc = GB_TRUE;
	}

	return 0;
}

int GB_loadsave(struct GB_Data* gb, const struct GB_SaveData* save) {
	assert(gb && save);

	if (!GB_has_save(gb)) {
		printf("[GB-ERROR] trying to loadsave when cart doesn't support battery ram!\n");
		return -1;
	}

	/* larger sizes would techinally be fine... */
	const GB_U32 wanted_size = GB_calculate_savedata_size(gb);
	if (save->size != wanted_size) {
		printf("[GB-ERROR] wrong wanted savesize. got: %u wanted %u\n", save->size, wanted_size);
		return -1;
	}

	// copy of the savedata!
	memcpy(gb->cart.ram, save->data, save->size);

	if (GB_has_rtc(gb) && save->has_rtc) {
		memcpy(&gb->cart.rtc, &save->rtc, sizeof(save->rtc));
	}

	return 0;
}

static const struct GB_StateHeader STATE_HEADER = {
	.magic = 1,
	.version = 1,
	/* padding */
};

int GB_savestate(const struct GB_Data* gb, struct GB_State* state) {
	if (!gb || !state) {
		return -1;
	}

	memcpy(&state->header, &STATE_HEADER, sizeof(state->header));
	return GB_savestate2(gb, &state->core, GB_TRUE);
}

int GB_loadstate(struct GB_Data* gb, const struct GB_State* state) {
	if (!gb || !state) {
		return -1;
	}

	// check if header is valid.
	// todo: maybe check each field.
	if (memcmp(&STATE_HEADER, &state->header, sizeof(STATE_HEADER)) != 0) {
		return -2;
	}

	return GB_loadstate2(gb, &state->core);
}

int GB_savestate2(const struct GB_Data* gb, struct GB_CoreState* state, GB_BOOL swap_endian) {
	if (!gb || !state) {
		return -1;
	}

	GB_UNUSED(swap_endian);

	memcpy(state->io, gb->io, sizeof(state->io));
	memcpy(state->hram, gb->hram, sizeof(state->hram));
	memcpy(state->wram, gb->wram, sizeof(state->wram));
	memcpy(&state->cpu, &gb->cpu, sizeof(state->cpu));
	memcpy(&state->ppu, &gb->ppu, sizeof(state->ppu));
	memcpy(&state->timer, &gb->timer, sizeof(state->timer));

	// todo: make this part of normal struct so that i can just memcpy
	memcpy(&state->cart.rom_bank, &gb->cart.rom_bank, sizeof(state->cart.rom_bank));
	memcpy(&state->cart.ram_bank, &gb->cart.ram_bank, sizeof(state->cart.ram_bank));
	memcpy(state->cart.ram, gb->cart.ram, sizeof(state->cart.ram));
	memcpy(&state->cart.rtc, &gb->cart.rtc, sizeof(state->cart.rtc));
	memcpy(&state->cart.bank_mode, &gb->cart.bank_mode, sizeof(state->cart.bank_mode));
	memcpy(&state->cart.ram_enabled, &gb->cart.ram_enabled, sizeof(state->cart.ram_enabled));
	memcpy(&state->cart.in_ram, &gb->cart.in_ram, sizeof(state->cart.in_ram));

	return 0;
}

int GB_loadstate2(struct GB_Data* gb, const struct GB_CoreState* state) {
	if (!gb || !state) {
		return -1;
	}

	memcpy(gb->io, state->io, sizeof(gb->io));
	memcpy(gb->hram, state->hram, sizeof(gb->hram));
	memcpy(gb->wram, state->wram, sizeof(gb->wram));
	memcpy(&gb->cpu, &state->cpu, sizeof(gb->cpu));
	memcpy(&gb->ppu, &state->ppu, sizeof(gb->ppu));
	memcpy(&gb->timer, &state->timer, sizeof(gb->timer));

	// setup cart
	memcpy(&gb->cart.rom_bank, &state->cart.rom_bank, sizeof(state->cart.rom_bank));
	memcpy(&gb->cart.ram_bank, &state->cart.ram_bank, sizeof(state->cart.ram_bank));
	memcpy(gb->cart.ram, state->cart.ram, sizeof(state->cart.ram));
	memcpy(&gb->cart.rtc, &state->cart.rtc, sizeof(state->cart.rtc));
	memcpy(&gb->cart.bank_mode, &state->cart.bank_mode, sizeof(state->cart.bank_mode));
	memcpy(&gb->cart.ram_enabled, &state->cart.ram_enabled, sizeof(state->cart.ram_enabled));
	memcpy(&gb->cart.in_ram, &state->cart.in_ram, sizeof(state->cart.in_ram));

	// we need to reload mmaps
	GB_setup_mmap(gb);

	return 0;
}

void GB_get_rom_info(const struct GB_Data* gb, struct GB_RomInfo* info) {
	assert(gb && info && gb->cart.rom);

	// const struct GB_CartHeader* header = GB_get_rom_header(gb);
	info->mbc_flags = gb->cart.flags;
	info->rom_size = gb->cart.rom_size;
	info->ram_size = gb->cart.ram_size;
}

void GB_enable_interrupt(struct GB_Data* gb, const enum GB_Interrupts interrupt) {
	IO_IF |= interrupt;
}

void GB_disable_interrupt(struct GB_Data* gb, const enum GB_Interrupts interrupt) {
	IO_IF &= ~(interrupt);
}

void GB_set_vblank_callback(struct GB_Data* gb, GB_vblank_callback_t cb, void* user_data) {
	gb->vblank_cb = cb;
	gb->vblank_cb_user_data = user_data;
}

void GB_set_hblank_callback(struct GB_Data* gb, GB_hblank_callback_t cb, void* user_data) {
	gb->hblank_cb = cb;
	gb->hblank_cb_user_data = user_data;
}

void GB_set_dma_callback(struct GB_Data* gb, GB_dma_callback_t cb, void* user_data) {
	gb->dma_cb = cb;
	gb->dma_cb_user_data = user_data;
}

void GB_set_halt_callback(struct GB_Data* gb, GB_halt_callback_t cb, void* user_data) {
	gb->halt_cb = cb;
	gb->halt_cb_user_data = user_data;
}

void GB_set_stop_callback(struct GB_Data* gb, GB_stop_callback_t cb, void* user_data) {
	gb->stop_cb = cb;
	gb->stop_cb_user_data = user_data;
}

void GB_set_error_callback(struct GB_Data* gb, GB_error_callback_t cb, void* user_data) {
	gb->error_cb = cb;
	gb->error_cb_user_data = user_data;
}

void GB_connect_link_cable(struct GB_Data* gb, GB_serial_transfer_t cb, void* user_data) {
	gb->link_cable = cb;
	gb->link_cable_user_data = user_data;
}

GB_U16 GB_run_step(struct GB_Data* gb) {
	GB_U16 cycles = GB_cpu_run(gb, 0 /*unused*/);
	GB_timer_run(gb, cycles);
	GB_ppu_run(gb, cycles);
	return cycles;
}

void GB_run_frame(struct GB_Data* gb) {
	assert(gb);

	GB_U32 cycles_elapsed = 0;

	do {
		cycles_elapsed += GB_run_step(gb);

	} while (cycles_elapsed < GB_FRAME_CPU_CYCLES);
}
