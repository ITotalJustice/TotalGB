#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

// these internally discard the const when passing the gb
// struct to the error callback.
// this function is still marked const because it is often called
// in const functions.
void GB_throw_info(const struct GB_Data* gb, const char* message);
void GB_throw_warn(const struct GB_Data* gb, const char* message);
void GB_throw_error(const struct GB_Data* gb, enum GB_ErrorDataType type, const char* message);

GB_U8 GB_ioread(struct GB_Data* gb, GB_U16 addr);
void GB_iowrite(struct GB_Data* gb, GB_U16 addr, GB_U8 value);
GB_U8 GB_read8(struct GB_Data* gb, const GB_U16 addr);
void GB_write8(struct GB_Data* gb, GB_U16 addr, GB_U8 value);
GB_U16 GB_read16(struct GB_Data* gb, GB_U16 addr);
void GB_write16(struct GB_Data* gb, GB_U16 addr, GB_U16 value);

GB_U8 GB_serial_sb_read(const struct GB_Data* gb);
void GB_serial_sc_write(struct GB_Data* gb, const GB_U8 data);

// these should also be static
GB_BOOL GB_setup_mbc(struct GB_Cart* mbc, const struct GB_CartHeader* header);
void GB_setup_mmap(struct GB_Data* gb);
void GB_update_rom_banks(struct GB_Data* gb);
void GB_update_ram_banks(struct GB_Data* gb);
void GB_update_vram_banks(struct GB_Data* gb);
void GB_update_wram_banks(struct GB_Data* gb);

// used internally
void GB_DMA(struct GB_Data* gb);
void GB_draw_scanline(struct GB_Data* gb);
void GB_update_all_colours_gb(struct GB_Data* gb);
void GB_set_coincidence_flag(struct GB_Data* gb, const GB_BOOL n);

void GB_set_status_mode(struct GB_Data* gb, const enum GB_StatusModes mode);
enum GB_StatusModes GB_get_status_mode(const struct GB_Data* gb);

void GB_compare_LYC(struct GB_Data* gb);
GB_U8 GB_joypad_get(const struct GB_Data* gb);

void GB_enable_interrupt(struct GB_Data* gb, const enum GB_Interrupts interrupt);
void GB_disable_interrupt(struct GB_Data* gb, const enum GB_Interrupts interrupt);

// used internally
GB_U16 GB_cpu_run(struct GB_Data* gb, GB_U16 cycles);
void GB_timer_run(struct GB_Data* gb, GB_U16 cycles);
void GB_ppu_run(struct GB_Data* gb, GB_U16 cycles);

GB_BOOL GB_is_lcd_enabled(const struct GB_Data* gb);
GB_BOOL GB_is_win_enabled(const struct GB_Data* gb);
GB_BOOL GB_is_obj_enabled(const struct GB_Data* gb);
GB_BOOL GB_is_bg_enabled(const struct GB_Data* gb);

#ifdef __cplusplus
}
#endif
