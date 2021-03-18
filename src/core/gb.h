#pragma once
// GB doesn't rely on any stb headers apart from stddef and assert, however,
// it does use the following by default:
// stdint.h, stdlib.h, string.h.
// these can be disabled, but you must provide your own function
// replacements (ie memcpy, memset, memcmp).

// OPTIONS:

// GB_NO_DYNAMIC_MEMORY:
// disable GB_alloc and GB_free and all dynamic alloc functions

// GB_NO_STDLIB:
// disable stdlib.h.
// you must write GB_alloc and GB_free is GB_NO_DYNAMIC_MEMORY is not set.

// GB_NO_STDINT:
// disable stdint.h.
// this will use char, short, int for U8, U16, U32 respectively.
// you must ensure that char == 1 byte and short == 2 bytes!

// GB_NO_STRING:
// disable string.h
// you must provide your own GB_memset, GB_memcpy and GB_memcmp functions.

// GB_ENDIAN: (autodetect by default)
// set the system endianess.
// set this to either GB_LITTLE_ENDIAN or GB_BIG_ENDIAN

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

GB_BOOL GB_init(struct GB_Data* gb);
void GB_quit(struct GB_Data* gb);
void GB_reset(struct GB_Data* gb);

// pass the fully loaded rom data, along with *optional* free function.
int GB_loadrom_data(struct GB_Data* gb, GB_U8* data, GB_U32 size, void(*free_func)(void*));
// pass in a filled in IFile struct to load the data and a data buffer large
// enough to load the rom into.
int GB_loadrom_ifile_with_data(struct GB_Data* gb, struct GB_IFile* ifile, GB_U8* data, GB_U32 size, void(*free_func)(void*));
// pass in a filled in IFile struct which will call GB_alloc for the
// rom data to be loaded and GB_free to later free it.
#ifndef GB_NO_DYNAMIC_MEMORY
int GB_loadrom_ifile(struct GB_Data* gb, struct GB_IFile* ifile, GB_U32 max_size);
#endif /* GB_NO_DYNAMIC_MEMORY */

GB_BOOL GB_has_save(const struct GB_Data* gb);
GB_BOOL GB_has_rtc(const struct GB_Data* gb);
GB_U32 GB_calculate_savedata_size(const struct GB_Data* gb);
GB_U32 GB_calculate_state_size(const struct GB_Data* gb);

int GB_savegame(const struct GB_Data* gb, struct GB_SaveData* save);
int GB_loadsave(struct GB_Data* gb, const struct GB_SaveData* save);

// save/loadstate to struct.
// header info is written / read and endianess is swapped on save.
int GB_savestate(const struct GB_Data* gb, struct GB_State* state);
int GB_loadstate(struct GB_Data* gb, const struct GB_State* state);

// like save/loadstate but does not fill out the header.
// setting swap_endian=true will swap the endian if the system is NOT little-endian.
// if swap_endian=false or the system is little-endian, it is not swapped.
// this is to be used if creating a rewind buffer as endianess and headers
// are not needed.
int GB_savestate2(const struct GB_Data* gb, struct GB_CoreState* state, GB_BOOL swap_endian);
int GB_loadstate2(struct GB_Data* gb, const struct GB_CoreState* state);

// int GB_set_colour_mode(struct GB_Data* gb, enum GB_ColourMode mode);
// enum GB_ColourMode GB_get_colour_mode(const struct GB_Data* gb);

void GB_run_test(struct GB_Data* gb);
void GB_run_frame(struct GB_Data* gb);

void GB_apu_init(struct GB_Data* gb);
void GB_apu_exit(struct GB_Data* gb);

/* returns a pointer to the loaded rom data as a GB_CartHeader */
const struct GB_CartHeader* GB_get_rom_header(const struct GB_Data* gb);


void GB_get_rom_info(const struct GB_Data* gb, struct GB_RomInfo* info);

/* set a callback which will be called when vsync happens. */
/* set the cb param to NULL to remove the callback */
void GB_set_vsync_callback(struct GB_Data* gb, GB_vsync_callback_t cb);

/* set a callback which will be called when hblank happens. */
/* set the cb param to NULL to remove the callback */
void GB_set_hblank_callback(struct GB_Data* gb, GB_hblank_callback_t cb);

/* set a callback which will be called when dma happens. */
/* set the cb param to NULL to remove the callback */
void GB_set_dma_callback(struct GB_Data* gb, GB_dma_callback_t cb);

GB_BOOL GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, GB_U8* hash);

GB_BOOL GB_get_rom_palette_hash(const struct GB_Data* gb, GB_U8* hash);

GB_BOOL GB_set_palette_from_table_entry(struct GB_Data* gb, GB_U8 table, GB_U8 entry);

GB_BOOL GB_set_palette_from_hash(struct GB_Data* gb, GB_U8 hash);

GB_BOOL GB_set_palette_from_buttons(struct GB_Data* gb, GB_U8 buttons);

/* set a custom palette, must be BGR555 format */
GB_BOOL GB_set_palette_from_palette(struct GB_Data* gb, const struct GB_PaletteEntry* palette);

// void GB_rollback_init_with_memory(struct GB_Data* gb, struct GB_CoreState* state, GB_U8 count, void (*free_func)(void*));
// #ifndef GB_NO_DYNAMIC_MEMORY
// void GB_rollback_init(struct GB_Data* gb, GB_U8 count);
// #endif /* GB_NO_DYNAMIC_MEMORY */
// void GB_rollback_exit(struct GB_Data* gb);

// this can be used to set multiple buttons down or up
// at once, or can set just 1.
void GB_set_buttons(struct GB_Data* gb, GB_U8 buttons, GB_BOOL is_down);
GB_U8 GB_get_buttons(const struct GB_Data* gb);
GB_BOOL GB_is_button_down(const struct GB_Data* gb, enum GB_Button button);

void GB_cpu_set_flag(struct GB_Data* gb, enum GB_CpuFlags flag, GB_BOOL value);
GB_BOOL GB_cpu_get_flag(const struct GB_Data* gb, enum GB_CpuFlags flag);

void GB_cpu_set_register(struct GB_Data* gb, enum GB_CpuRegisters reg, GB_U8 value);
GB_U8 GB_cpu_get_register(const struct GB_Data* gb, enum GB_CpuRegisters reg);

void GB_cpu_set_register_pair(struct GB_Data* gb, enum GB_CpuRegisterPairs pair, GB_U16 value);
GB_U16 GB_cpu_get_register_pair(const struct GB_Data* gb, enum GB_CpuRegisterPairs pair);

#ifdef __cplusplus
}
#endif
