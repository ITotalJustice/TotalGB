#ifndef _GB_H_
#define _GB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"


bool GB_PUBLIC GB_init(struct GB_Core* gb);
void GB_PUBLIC GB_quit(struct GB_Core* gb);
void GB_PUBLIC GB_reset(struct GB_Core* gb);

// pass the fully loaded rom data.
// this memory is NOT owned.
// freeing the memory should still be handled by the caller!
bool GB_PUBLIC GB_loadrom(struct GB_Core* gb, const uint8_t* data, size_t size);

bool GB_PUBLIC GB_has_save(const struct GB_Core* gb);
bool GB_PUBLIC GB_has_rtc(const struct GB_Core* gb);

uint32_t GB_PUBLIC GB_calculate_savedata_size(const struct GB_Core* gb);
uint32_t GB_PUBLIC GB_calculate_state_size(const struct GB_Core* gb);

bool GB_PUBLIC GB_savegame(const struct GB_Core* gb, struct GB_SaveData* save);
bool GB_PUBLIC GB_loadsave(struct GB_Core* gb, const struct GB_SaveData* save);

// save/loadstate to struct.
// header info is written / read and endianess is swapped on save.
bool GB_PUBLIC GB_savestate(const struct GB_Core* gb, struct GB_State* state);
bool GB_PUBLIC GB_loadstate(struct GB_Core* gb, const struct GB_State* state);

// like save/loadstate but does not fill out the header.
// this is to be used if creating a rewind buffer as endianess and headers
// are not needed.
bool GB_PUBLIC GB_savestate2(const struct GB_Core* gb, struct GB_CoreState* state);
bool GB_PUBLIC GB_loadstate2(struct GB_Core* gb, const struct GB_CoreState* state);

// int GB_set_colour_mode(struct GB_Core* gb, enum GB_ColourMode mode);
// enum GB_ColourMode GB_get_colour_mode(const struct GB_Core* gb);

// pass in a fully filled out rtc struct.
// NOTE: the s, m, h will be clamped to the max values
// so there won't be 255 seconds, it'll be clamped to 59.
// returns false if the game does not support rtc.
bool GB_PUBLIC GB_set_rtc(struct GB_Core* gb, const struct GB_Rtc rtc);

// this will work even if the game does NOT have RTC
// this setting persits accross games!
void GB_PUBLIC GB_set_rtc_update_config(struct GB_Core* gb, const enum GB_RtcUpdateConfig config);

bool GB_PUBLIC GB_has_mbc_flags(const struct GB_Core* gb, const uint8_t flags);

/* returns the number of cycles ran */
uint16_t GB_PUBLIC GB_run_step(struct GB_Core* gb);

/* run until the end of a frame */
void GB_PUBLIC GB_run_frame(struct GB_Core* gb);

void GB_PUBLIC GB_set_render_palette_layer_config(struct GB_Core* gb, enum GB_RenderLayerConfig layer);

enum GB_SystemType GB_PUBLIC GB_PUBLIC GB_get_system_type(const struct GB_Core* gb);

// calls GB_get_system_type(gb) and compares the result
bool GB_PUBLIC GB_is_system_gbc(const struct GB_Core* gb);

// fills out the header struct using the loaded rom data
bool GB_PUBLIC GB_get_rom_header(const struct GB_Core* gb, struct GB_CartHeader* header);

/* returns a pointer to the loaded rom data as a GB_CartHeader */
// this can be used to modify the contents of the header,
// useful for if you plan to save the rom after to a new file.
struct GB_CartHeader GB_PUBLIC* GB_get_rom_header_ptr(const struct GB_Core* gb);

void GB_PUBLIC GB_get_rom_info(const struct GB_Core* gb, struct GB_RomInfo* info);

/*  */
int GB_PUBLIC GB_get_rom_name(const struct GB_Core* gb, struct GB_CartName* name);
int GB_PUBLIC GB_get_rom_name_from_header(const struct GB_CartHeader* header, struct GB_CartName* name);

/* set a callback which will be called when apu has filled 512 stero samples. */
/* not setting this callback is valid, just that you won't have audio... */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_apu_callback(struct GB_Core* gb, struct GB_AudioCallbackData* data);

/* set a callback which will be called when vblank happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_vblank_callback(struct GB_Core* gb, GB_vblank_callback_t cb, void* user_data);

/* set a callback which will be called when hblank happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_hblank_callback(struct GB_Core* gb, GB_hblank_callback_t cb, void* user_data);

/* set a callback which will be called when dma happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_dma_callback(struct GB_Core* gb, GB_dma_callback_t cb, void* user_data);

/* set a callback which will be called when halt happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_halt_callback(struct GB_Core* gb, GB_halt_callback_t cb, void* user_data);

/* set a callback which will be called when stop happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_stop_callback(struct GB_Core* gb, GB_stop_callback_t cb, void* user_data);

/* set a callback which will be called when error happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_set_error_callback(struct GB_Core* gb, GB_error_callback_t cb, void* user_data);

/* set a callback which will be called when link transfer happens. */
/* set the cb param to NULL to remove the callback */
void GB_PUBLIC GB_connect_link_cable(struct GB_Core* gb, GB_serial_transfer_t cb, void* user_data);

// this sets the link cable callback to an interal function.
// the user_data for the callback will be the passed in gb struct.
// WARNING: this will overwrite the exisitng link_cable_cb and user_data!
void GB_PUBLIC GB_connect_link_cable_builtin(struct GB_Core* gb, struct GB_Core* gb2);

bool GB_PUBLIC GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, uint8_t* hash, uint8_t* forth);

bool GB_PUBLIC GB_get_rom_palette_hash(const struct GB_Core* gb, uint8_t* hash, uint8_t* forth);

bool GB_PUBLIC GB_set_palette_from_table_entry(struct GB_Core* gb, uint8_t table, uint8_t entry);

bool GB_PUBLIC GB_set_palette_from_hash(struct GB_Core* gb, uint8_t hash);

bool GB_PUBLIC GB_set_palette_from_buttons(struct GB_Core* gb, uint8_t buttons);

/* set a custom palette, must be BGR555 format */
bool GB_PUBLIC GB_set_palette_from_palette(struct GB_Core* gb, const struct GB_PaletteEntry* palette);

// this can be used to set multiple buttons down or up
// at once, or can set just 1.
void GB_PUBLIC GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down);
uint8_t GB_PUBLIC GB_get_buttons(const struct GB_Core* gb);
bool GB_PUBLIC GB_is_button_down(const struct GB_Core* gb, enum GB_Button button);

void GB_PUBLIC GB_cpu_set_flag(struct GB_Core* gb, enum GB_CpuFlags flag, bool value);
bool GB_PUBLIC GB_cpu_get_flag(const struct GB_Core* gb, enum GB_CpuFlags flag);

void GB_PUBLIC GB_cpu_set_register(struct GB_Core* gb, enum GB_CpuRegisters reg, uint8_t value);
uint8_t GB_PUBLIC GB_cpu_get_register(const struct GB_Core* gb, enum GB_CpuRegisters reg);

void GB_PUBLIC GB_cpu_set_register_pair(struct GB_Core* gb, enum GB_CpuRegisterPairs pair, uint16_t value);
uint16_t GB_PUBLIC GB_cpu_get_register_pair(const struct GB_Core* gb, enum GB_CpuRegisterPairs pair);


// logs each cpu instruction to stdout
// this only does something if built with GB_DEBUG=1.
void GB_PUBLIC GB_cpu_enable_log(const bool enable);

#ifdef __cplusplus
}
#endif

#endif // _GB_H_
