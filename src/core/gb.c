#include "gb.h"
#include "tables/palette_table.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>

#define ROM_SIZE_MULT 0x8000

#if !defined GB_NO_STDLIB
#include <stdlib.h>
#endif

#if !defined GB_NO_STRING
#include <string.h>
int GB_memcmp(const void* a, const void* b, size_t size) { return memcmp(a, b, size); }
void* GB_memset(void* ptr, int value, size_t size) { return memset(ptr, value, size); }
void* GB_memcpy(void* dst, const void* src, size_t size) { return memcpy(dst, src, size); }
#endif

#if !defined GB_NO_DYNAMIC_MEMORY  && !defined GB_NO_STDLIB && !defined GB_CUSTOM_ALLOC
void* GB_alloc(size_t size) { return malloc(size); }
void GB_free(void* ptr) { free(ptr); }
#endif

GB_BOOL GB_init(struct GB_Data* gb) {
	if (!gb) {
		return GB_FALSE;
	}

	GB_memset(&gb->rollback, 0, sizeof(gb->rollback));
	gb->cart.mbc_type = MBC_TYPE_NONE;
	GB_Palette_fill_from_custom(GB_CUSTOM_PALETTE_CREAM, &gb->palette);
	gb->vsync_cb = NULL;
	gb->hblank_cb = NULL;
	GB_apu_init(gb);

	return GB_TRUE;
}

void GB_quit(struct GB_Data* gb) {
	assert(gb);

	GB_apu_exit(gb);
	
	#define FREE_ELSE_WARN(name, func) \
	if (gb->cart.mbc.name != NULL) { \
		if (func != NULL) { \
			func(gb->cart.mbc.name); \
			gb->cart.mbc.name = NULL; \
		} else { \
			printf("[GB-WARN] Leaking %s\n", #name); \
		} \
	}

	FREE_ELSE_WARN(rom, gb->rom_free_func);
}

void GB_reset(struct GB_Data* gb) {
	GB_memset(gb->wram, 0, sizeof(gb->wram));
	GB_memset(gb->hram, 0, sizeof(gb->hram));
	GB_memset(IO, 0xFF, sizeof(IO));
	GB_memset(gb->ppu.vram, 0, sizeof(gb->ppu.vram));
	GB_memset(gb->ppu.bg_palette, 0, sizeof(gb->ppu.bg_palette));
	GB_memset(gb->ppu.obj_palette, 0, sizeof(gb->ppu.obj_palette));
	GB_memset(gb->ppu.pixles, 0, sizeof(gb->ppu.pixles));
	GB_memset(gb->ppu.bg_colours, 0, sizeof(gb->ppu.bg_colours));
	GB_memset(gb->ppu.obj_colours, 0, sizeof(gb->ppu.obj_colours));
	GB_memset(gb->ppu.dirty_bg, 0, sizeof(gb->ppu.dirty_bg));
	GB_memset(gb->ppu.dirty_obj, 0, sizeof(gb->ppu.dirty_obj));
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
	GB_U8 hash;
	GB_get_rom_palette_hash_from_header(header, &hash);
	printf("HASH: 0x%02X\n", hash);
    putchar('\n');
}

static const struct GB_CartHeader* GB_get_rom_header_from_data(const GB_U8* data) {
	assert(data);
	return (struct GB_CartHeader*)&data[GB_BOOTROM_SIZE];
}

const struct GB_CartHeader* GB_get_rom_header(const struct GB_Data* gb) {
	assert(gb && gb->cart.mbc.rom);
	return GB_get_rom_header_from_data(gb->cart.mbc.rom);
}

GB_BOOL GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, GB_U8* hash) {
	assert(header && hash);

	if (!header || !hash) {
		return GB_FALSE;
	}

	GB_U16 temp_hash = 0;
	for (GB_U16 i = 0; i < sizeof(header->title); ++i) {
		temp_hash += header->title[i];
	}

	*hash = temp_hash & 0xFF;

	return GB_TRUE;
}

GB_BOOL GB_get_rom_palette_hash(const struct GB_Data* gb, GB_U8* hash) {
	assert(gb && hash);

	if (!gb || !hash) {
		return GB_FALSE;
	}

	return GB_get_rom_palette_hash_from_header(
		GB_get_rom_header(gb),
		hash
	);
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
	if (rom_size > (size)) {
		return -1;
	}

	gb->cart.mbc.rom_size = rom_size;
	gb->cart.mbc.rom = data;

	if (!GB_setup_mbc(&gb->cart.mbc, header)) {
		return -1;
	}

	GB_reset(gb);
	GB_setup_mmap(gb);

	gb->rom_free_func = free_func;
	return 0;
}

int GB_loadrom_ifile_with_data(struct GB_Data* gb, struct GB_IFile* ifile, GB_U8* data, GB_U32 data_size, void(*free_func)(void*)) {
	if (!gb || !ifile || !data || !data_size || data_size < GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader)) {
		goto err_exit;
	}

	// todo: handle errors.

	// might as well read bootrom now rather than seek + read header,
	// then seek back, read bootrom + header + rom.
	// instead its read bootrom + header, then read rom, no seek.
	GB_U32 offset = 0;
	ifile->iread(ifile->handle, data, GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader));

	offset += GB_BOOTROM_SIZE + GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader);
	const struct GB_CartHeader* header = GB_get_rom_header_from_data(data);

	const GB_U32 rom_size = ROM_SIZE_MULT << header->rom_size;
	if (rom_size > (data_size - offset)) {
		goto err_exit;
	}

	// todo: handle errors
	ifile->iread(ifile->handle, data + offset, rom_size);

	gb->cart.mbc.rom_size = rom_size;
	gb->cart.mbc.rom = data;

	if (!GB_setup_mbc(&gb->cart.mbc, header)) {
		goto err_exit;
	}

	GB_reset(gb);
	GB_setup_mmap(gb);
	gb->rom_free_func = free_func;

	if (ifile) {
		ifile->iclose(ifile->handle);
	}
	return 0;

	err_exit:
	if (ifile) {
		ifile->iclose(ifile->handle);
	}
	return -1;
}

#ifndef GB_NO_DYNAMIC_MEMORY
int GB_loadrom_ifile(struct GB_Data* gb, struct GB_IFile* ifile, GB_U32 max_size) {
	if (!gb || !ifile) {
		return -1;
	}

	GB_U8 data[GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader)] = {0};
	const struct GB_CartHeader* header = GB_get_rom_header_from_data(data);
	ifile->iread(ifile->handle, data, sizeof(data));
	cart_header_print(header);

	const GB_U32 rom_size = ROM_SIZE_MULT << header->rom_size;
	if (rom_size > (max_size - sizeof(data))) {
		goto err_exit;
	}

	// todo: free if fail later on.
	GB_U8* rom_data = GB_alloc(rom_size);
	if (!rom_data) {
		goto err_exit;
	}

	GB_memcpy(rom_data, data, sizeof(data));

	// todo: handle errors
	ifile->iread(ifile->handle, rom_data + sizeof(data), rom_size - sizeof(data));

	gb->cart.mbc.rom_size = rom_size;
	gb->cart.mbc.rom = rom_data;

	if (!GB_setup_mbc(&gb->cart.mbc, header)) {
		goto err_exit;
	}

	GB_U8 hash;
	if (GB_get_rom_palette_hash_from_header(header, &hash)) {
		// GB_Palette_fill_from_custom(GB_CUSTOM_PALETTE_GREEN, &gb->palette);
		if (0 == GB_palette_fill_from_hash(hash, header->title[0x3], &gb->palette)) {
			printf("filled palette data\n");
		} else {
			printf("failed to find hash\n");
		}
	} else {
		printf("didnt fill palette data\n");
	}

	GB_reset(gb);
	GB_setup_mmap(gb);
	gb->rom_free_func = GB_free;

	if (ifile) {
		ifile->iclose(ifile->handle);
	}
	return 0;

	err_exit:
	if (ifile) {
		ifile->iclose(ifile->handle);
	}
	return -1;
}
#endif

GB_BOOL GB_has_save(const struct GB_Data* gb) {
	assert(gb);
	return (gb->cart.mbc.flags & (MBC_FLAGS_RAM | MBC_FLAGS_BATTERY)) > 0;
}

GB_BOOL GB_has_rtc(const struct GB_Data* gb) {
	assert(gb);
	return (gb->cart.mbc.flags & MBC_FLAGS_RTC) > 0;
}

GB_U32 GB_calculate_savedata_size(const struct GB_Data* gb) {
	assert(gb);
	return gb->cart.mbc.ram_size;
}

int GB_savegame(const struct GB_Data* gb, struct GB_SaveData* save) {
	assert(gb && save);

	if (!GB_has_save(gb)) {
		printf("[GB-ERROR] trying to savegame when cart doesn't support battery ram!\n");
		return -1;
	}

	/* larger sizes would techinally be fine... */
	save->size = GB_calculate_savedata_size(gb);
	GB_memcpy(save->data, gb->cart.mbc.ram, save->size);
	if (GB_has_rtc(gb) == GB_TRUE) {
		GB_memcpy(&save->rtc, &gb->cart.mbc.rtc, sizeof(save->rtc));
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
	GB_U32 wanted_size = GB_calculate_savedata_size(gb);
	if (save->size != wanted_size) {
		return -1;
	}

	GB_memcpy(gb->cart.mbc.ram, save->data, save->size);
	if (GB_has_rtc(gb) && save->has_rtc) {
		GB_memcpy(&gb->cart.mbc.rtc, &save->rtc, sizeof(save->rtc));
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

	GB_memcpy(&state->header, &STATE_HEADER, sizeof(state->header));
	return GB_savestate2(gb, &state->core, GB_TRUE);
}

int GB_loadstate(struct GB_Data* gb, const struct GB_State* state) {
	if (!gb || !state) {
		return -1;
	}

	// check if header is valid.
	// todo: maybe check each field.
	if (GB_memcmp(&STATE_HEADER, &state->header, sizeof(STATE_HEADER)) != 0) {
		return -2;
	}

	return GB_loadstate2(gb, &state->core);
}

int GB_savestate2(const struct GB_Data* gb, struct GB_CoreState* state, GB_BOOL swap_endian) {
	if (!gb || !state) {
		return -1;
	}

	GB_memcpy(state->io, gb->io, sizeof(state->io));
	GB_memcpy(state->hram, gb->hram, sizeof(state->hram));
	GB_memcpy(state->wram, gb->wram, sizeof(state->wram));
	GB_memcpy(&state->cpu, &gb->cpu, sizeof(state->cpu));
	GB_memcpy(&state->ppu, &gb->ppu, sizeof(state->ppu));
	GB_memcpy(&state->timer, &gb->timer, sizeof(state->timer));

	// todo: make this part of normal struct so that i can just memcpy
	GB_memcpy(&state->cart.rom_bank, &gb->cart.mbc.rom_bank, sizeof(state->cart.rom_bank));
	GB_memcpy(&state->cart.ram_bank, &gb->cart.mbc.ram_bank, sizeof(state->cart.ram_bank));
	GB_memcpy(state->cart.ram, gb->cart.mbc.ram, sizeof(state->cart.ram));
	GB_memcpy(&state->cart.rtc, &gb->cart.mbc.rtc, sizeof(state->cart.rtc));
	GB_memcpy(&state->cart.bank_mode, &gb->cart.mbc.bank_mode, sizeof(state->cart.bank_mode));
	GB_memcpy(&state->cart.ram_enabled, &gb->cart.mbc.ram_enabled, sizeof(state->cart.ram_enabled));
	GB_memcpy(&state->cart.in_ram, &gb->cart.mbc.in_ram, sizeof(state->cart.in_ram));

	return 0;
}

int GB_loadstate2(struct GB_Data* gb, const struct GB_CoreState* state) {
	if (!gb || !state) {
		return -1;
	}

	GB_memcpy(gb->io, state->io, sizeof(gb->io));
	GB_memcpy(gb->hram, state->hram, sizeof(gb->hram));
	GB_memcpy(gb->wram, state->wram, sizeof(gb->wram));
	GB_memcpy(&gb->cpu, &state->cpu, sizeof(gb->cpu));
	GB_memcpy(&gb->ppu, &state->ppu, sizeof(gb->ppu));
	GB_memcpy(&gb->timer, &state->timer, sizeof(gb->timer));

	// setup cart
	GB_memcpy(&gb->cart.mbc.rom_bank, &state->cart.rom_bank, sizeof(state->cart.rom_bank));
	GB_memcpy(&gb->cart.mbc.ram_bank, &state->cart.ram_bank, sizeof(state->cart.ram_bank));
	GB_memcpy(gb->cart.mbc.ram, state->cart.ram, sizeof(state->cart.ram));
	GB_memcpy(&gb->cart.mbc.rtc, &state->cart.rtc, sizeof(state->cart.rtc));
	GB_memcpy(&gb->cart.mbc.bank_mode, &state->cart.bank_mode, sizeof(state->cart.bank_mode));
	GB_memcpy(&gb->cart.mbc.ram_enabled, &state->cart.ram_enabled, sizeof(state->cart.ram_enabled));
	GB_memcpy(&gb->cart.mbc.in_ram, &state->cart.in_ram, sizeof(state->cart.in_ram));

	// we need to reload mmaps
	GB_setup_mmap(gb);

	return 0;
}

void GB_rollback_init_with_memory(struct GB_Data* gb, struct GB_CoreState* states, GB_U8 count, void (*free_func)(void*)) {
	assert(gb);
	assert(gb->rollback.enabled == GB_FALSE);
	assert(states && count);

	GB_memcpy(&gb->rollback.joypad, &gb->joypad, sizeof(gb->rollback.joypad));
	gb->rollback.free_func = free_func;
	gb->rollback.states = states;
	gb->rollback.position = 0;
	gb->rollback.states_since_init = 0;
	gb->rollback.count = count;
	gb->rollback.enabled = GB_TRUE;
}

#ifndef GB_NO_DYNAMIC_MEMORY
void GB_rollback_init(struct GB_Data* gb, GB_U8 count) {
	assert(gb && count);
	struct GB_CoreState* states = (struct GB_CoreState*)GB_alloc(count * sizeof(struct GB_CoreState));
	assert(states);

	GB_rollback_init_with_memory(gb, states, count, GB_free);
}
#endif

void GB_rollback_exit(struct GB_Data* gb) {
	assert(gb);
	assert(gb->rollback.enabled == GB_TRUE);

	if (gb->rollback.enabled == GB_TRUE) {
		if (gb->rollback.free_func != NULL) {
			gb->rollback.free_func(gb->rollback.states);
		}
		GB_memset(&gb->rollback, 0, sizeof(gb->rollback));
	}
}

void GB_get_rom_info(const struct GB_Data* gb, struct GB_RomInfo* info) {
	assert(gb && info && gb->cart.mbc.rom);

	// const struct GB_CartHeader* header = GB_get_rom_header(gb);
	info->mbc_flags = gb->cart.mbc.flags;
	info->rom_size = gb->cart.mbc.rom_size;
	info->ram_size = gb->cart.mbc.ram_size;
}

/* set a callback which will be called when vsync happens. */
/* set the cb param to NULL to remove the callback */
void GB_set_vsync_callback(struct GB_Data* gb, GB_vsync_callback_t cb) {
	gb->vsync_cb = cb;
}

/* set a callback which will be called when hblank happens. */
/* set the cb param to NULL to remove the callback */
void GB_set_hblank_callback(struct GB_Data* gb, GB_hblank_callback_t cb) {
	gb->hblank_cb = cb;
}

#ifdef FPS
#include <time.h>
#endif

void GB_run_test(struct GB_Data* gb) {
	const GB_U32 cycle_end = 70224;
	GB_U32 cycles_elapsed = 0;
	GB_U16 cycles = 0;
	#ifdef FPS
	GB_U32 fps = 0;
	time_t start = time(NULL);
	#endif

	while (1) {
		cycles = GB_cpu_run(gb, 0 /*unused*/);
		GB_timer_run(gb, cycles);
		GB_ppu_run(gb, cycles);
		cycles_elapsed += cycles;

		if (UNLIKELY(cycles_elapsed >= cycle_end)) {
			cycles_elapsed = 0;
			#ifdef FPS
			fps++;
			time_t now = time(NULL);
			if (difftime(now, start) >= 1.0) {
				printf("FPS: %u\n", fps);
				start = now;
				fps = 0;
			}
			#endif
		}
	}
}

void GB_run_frame(struct GB_Data* gb) {
	assert(gb);

	static const GB_U32 cycle_end = 70224;
	GB_U32 cycles_elapsed = 0;
	GB_U16 cycles = 0;

	do {
		cycles = GB_cpu_run(gb, 0 /*unused*/);
		GB_timer_run(gb, cycles);
		GB_ppu_run(gb, cycles);
		cycles_elapsed += cycles;

	} while (cycles_elapsed < cycle_end);
}
