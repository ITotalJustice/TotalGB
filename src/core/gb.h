#ifndef GB_H
#define GB_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"


GBAPI bool GB_init(struct GB_Core* gb);
GBAPI void GB_quit(struct GB_Core* gb);
GBAPI void GB_reset(struct GB_Core* gb);

// set the pixels that the game will render to
// IMPORTANT: if pixels == NULL, then no rendering will happen!
GBAPI void GB_set_pixels(struct GB_Core* gb, void* pixels, uint32_t stride, uint8_t bpp);

// todo: explain this function
GBAPI void GB_set_sram(struct GB_Core* gb, uint8_t* ram, size_t size);

// todo: explain this function
GBAPI bool GB_get_rom_info(const uint8_t* data, size_t size, struct GB_RomInfo* info_out);

// pass the fully loaded rom data.
// this memory is NOT owned.
// freeing the memory should still be handled by the caller!
GBAPI bool GB_loadrom(struct GB_Core* gb, const uint8_t* data, size_t size);

GBAPI bool GB_has_save(const struct GB_Core* gb);
GBAPI bool GB_has_rtc(const struct GB_Core* gb);

GBAPI size_t GB_calculate_savedata_size(const struct GB_Core* gb);

// save/loadstate to struct.
GBAPI bool GB_savestate(const struct GB_Core* gb, struct GB_State* state);
GBAPI bool GB_loadstate(struct GB_Core* gb, const struct GB_State* state);

// pass in filled out rtc struct.
// NOTE: the s, m, h will be clamped to the max values
// so there won't be 255 seconds, it'll be clamped to 59.
// returns false if the game does not support rtc.
GBAPI bool GB_set_rtc(struct GB_Core* gb, const struct GB_Rtc rtc);

// this will work even if the game does NOT have RTC
// this setting persits accross games!
GBAPI void GB_set_rtc_update_config(struct GB_Core* gb, const enum GB_RtcUpdateConfig config);

GBAPI bool GB_has_mbc_flags(const struct GB_Core* gb, const uint8_t flags);

/* run for number of cycles */
GBAPI void GB_run(struct GB_Core* gb, uint32_t tcycles);

GBAPI enum GB_SystemType GB_get_system_type(const struct GB_Core* gb);

// calls GB_get_system_type(gb) and compares the result
GBAPI bool GB_is_system_gbc(const struct GB_Core* gb);

/*  */
GBAPI int GB_get_rom_name(const struct GB_Core* gb, struct GB_CartName* name);
GBAPI int GB_get_rom_name_from_header(const struct GB_CartHeader* header, struct GB_CartName* name);

GBAPI void GB_set_apu_freq(struct GB_Core* gb, unsigned freq);

/* set a callback which will be called when apu has filled 512 stero samples. */
/* not setting this callback is valid, just that you won't have audio... */
GBAPI void GB_set_apu_callback(struct GB_Core* gb, GB_apu_callback_t cb, void* user, unsigned freq);

/* set a callback which will be called when vblank happens. */
GBAPI void GB_set_vblank_callback(struct GB_Core* gb, GB_vblank_callback_t cb, void* user);

/* set a callback which will be called when hblank happens. */
GBAPI void GB_set_hblank_callback(struct GB_Core* gb, GB_hblank_callback_t cb, void* user);

/* set a callback which will be called when dma happens. */
GBAPI void GB_set_dma_callback(struct GB_Core* gb, GB_dma_callback_t cb, void* user);

/* set a callback which will be called when halt happens. */
GBAPI void GB_set_halt_callback(struct GB_Core* gb, GB_halt_callback_t cb, void* user);

/* set a callback which will be called when stop happens. */
GBAPI void GB_set_stop_callback(struct GB_Core* gb, GB_stop_callback_t cb, void* user);

GBAPI void GB_set_colour_callback(struct GB_Core* gb, GB_colour_callback_t cb, void* user);

GBAPI void GB_set_rom_bank_callback(struct GB_Core* gb, GB_rom_bank_callback_t cb, void* user);

/* set a callback which will be called when link transfer happens. */
/* set the cb param to NULL to remove the callback */
GBAPI void GB_connect_link_cable(struct GB_Core* gb, GB_serial_transfer_t cb, void* user);

// this sets the link cable callback to an interal function.
// the user_data for the callback will be the passed in gb struct.
// WARNING: this will overwrite the exisitng link_cable_cb and user_data!
GBAPI void GB_connect_link_cable_builtin(struct GB_Core* gb, struct GB_Core* gb2);

// this can be used to set multiple buttons down or up
// at once, or can set just 1.
GBAPI void GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down);
GBAPI uint8_t GB_get_buttons(const struct GB_Core* gb);
GBAPI bool GB_is_button_down(const struct GB_Core* gb, enum GB_Button button);

// make this a seperate header, gb_adv.h, add these there
GBAPI bool GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, uint8_t* hash, uint8_t* forth);

GBAPI bool GB_get_rom_palette_hash(const struct GB_Core* gb, uint8_t* hash, uint8_t* forth);

GBAPI bool GB_set_palette_from_table_entry(struct GB_Core* gb, uint8_t table, uint8_t entry);

GBAPI bool GB_set_palette_from_hash(struct GB_Core* gb, uint8_t hash);

// todo: show button table here or link to the table!
GBAPI bool GB_set_palette_from_buttons(struct GB_Core* gb, uint8_t buttons);

/* set a custom palette */
GBAPI bool GB_set_palette_from_palette(struct GB_Core* gb, const struct GB_PaletteEntry* palette);




/* -------------------- ADVANCED STUFF -------------------- */


// fills out the header struct using the loaded rom data
GBAPI bool GB_get_rom_header(const struct GB_Core* gb, struct GB_CartHeader* header);

/* returns a pointer to the loaded rom data as a GB_CartHeader */
GBAPI const struct GB_CartHeader* GB_get_rom_header_ptr(const struct GB_Core* gb);

#if GB_DEBUG
    /* set a callback which will be called when cpu read happens. */
    GBAPI void GB_set_read_callback(struct GB_Core* gb, GB_read_callback_t cb, void* user);

    /* set a callback which will be called when cpu write happens. */
    GBAPI void GB_set_write_callback(struct GB_Core* gb, GB_write_callback_t cb, void* user);
#endif // GB_DEBUG

GBAPI uint8_t GB_read(struct GB_Core* gb, uint16_t addr);
GBAPI void GB_write(struct GB_Core* gb, uint16_t addr, uint8_t value);

GBAPI uint8_t GB_get_ram_bank(struct GB_Core* gb);
GBAPI uint8_t GB_get_wram_bank(struct GB_Core* gb);

GBAPI void GB_set_ram_bank(struct GB_Core* gb, uint8_t bank);
GBAPI void GB_set_wram_bank(struct GB_Core* gb, uint8_t bank);

GBAPI void GB_cpu_set_flag(struct GB_Core* gb, enum GB_CpuFlags flag, bool value);
GBAPI bool GB_cpu_get_flag(const struct GB_Core* gb, enum GB_CpuFlags flag);

GBAPI void GB_cpu_set_register(struct GB_Core* gb, enum GB_CpuRegisters reg, uint8_t value);
GBAPI uint8_t GB_cpu_get_register(const struct GB_Core* gb, enum GB_CpuRegisters reg);

GBAPI void GB_cpu_set_register_pair(struct GB_Core* gb, enum GB_CpuRegisterPairs pair, uint16_t value);
GBAPI uint16_t GB_cpu_get_register_pair(const struct GB_Core* gb, enum GB_CpuRegisterPairs pair);

GBAPI void GB_cpu_enable_log(const bool enable);

#ifdef __cplusplus
}
#endif

#endif // GB_H
