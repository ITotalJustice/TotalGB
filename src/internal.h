#ifndef _GB_INTERNAL_H_
#define _GB_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define GB_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define GB_MAX(x, y) (((x) > (y)) ? (x) : (y))

// msvc prepro has a hard time with (macro && macro), so they have to be
// split into different if, else chains...
#if defined(__has_builtin)
#if __has_builtin(__builtin_expect)
#define LIKELY(c) (__builtin_expect(c,1))
#define UNLIKELY(c) (__builtin_expect(c,0))
#else
#define LIKELY(c) ((c))
#define UNLIKELY(c) ((c))
#endif // __has_builtin(__builtin_expect)
#else
#define LIKELY(c) ((c))
#define UNLIKELY(c) ((c))
#endif // __has_builtin

#if defined(__has_builtin)
#if __has_builtin(__builtin_unreachable)
#define GB_UNREACHABLE(ret) __builtin_unreachable()
#else
#define GB_UNREACHABLE(ret) return ret
#endif // __has_builtin(__builtin_unreachable)
#else
#define GB_UNREACHABLE(ret) return ret
#endif // __has_builtin

// used mainly in debugging when i want to quickly silence
// the compiler about unsed vars.
#define GB_UNUSED(var) ((void)(var))

// ONLY use this for C-arrays, not pointers, not structs
#define GB_ARR_SIZE(array) (sizeof(array) / sizeof(array[0]))

#ifdef GB_DEBUG
#include <stdio.h>
#define GB_log(...) fprintf(stdout, __VA_ARGS__)
#define GB_log_err(...) fprintf(stderr, __VA_ARGS__)
#else
#define GB_log(...)
#define GB_log_err(...)
#endif // NES_DEBUG

// 4-mhz
#define DMG_CPU_CLOCK 4194304
// 8-mhz
#define GBC_CPU_CLOCK (DMG_CPU_CLOCK << 1)

#define IO gb->io
// JOYPAD
#define IO_JYP IO[0x00]
// SERIAL
#define IO_SB IO[0x01]
#define IO_SC IO[0x02]
// TIMERS
#define IO_DIV_LOWER IO[0x03]
#define IO_DIV_UPPER IO[0x04]
#define IO_TIMA IO[0x05]
#define IO_TMA IO[0x06]
#define IO_TAC IO[0x07]
// APU (square1)
#define IO_NR10 gb->apu.ch1
#define IO_NR11 gb->apu.ch1
#define IO_NR12 gb->apu.ch1
#define IO_NR13 gb->apu.ch1
#define IO_NR14 gb->apu.ch1
// APU (square2)
#define IO_NR21 gb->apu.ch2
#define IO_NR22 gb->apu.ch2
#define IO_NR23 gb->apu.ch2
#define IO_NR24 gb->apu.ch2
// APU (wave)
#define IO_NR30 gb->apu.ch3
#define IO_NR31 gb->apu.ch3
#define IO_NR32 gb->apu.ch3
#define IO_NR33 gb->apu.ch3
#define IO_NR34 gb->apu.ch3
#define IO_WAVE_TABLE (IO + 0x30)
// APU (noise)
#define IO_NR41 gb->apu.ch4
#define IO_NR42 gb->apu.ch4
#define IO_NR43 gb->apu.ch4
#define IO_NR44 gb->apu.ch4
// APU (control)
#define IO_NR50 gb->apu.control
#define IO_NR51 gb->apu.control
#define IO_NR52 gb->apu.control
// PPU
#define IO_LCDC IO[0x40]
#define IO_STAT IO[0x41]
#define IO_SCY IO[0x42]
#define IO_SCX IO[0x43]
#define IO_LY IO[0x44]
#define IO_LYC IO[0x45]
#define IO_DMA IO[0x46]
#define IO_BGP IO[0x47]
#define IO_OBP0 IO[0x48]
#define IO_OBP1 IO[0x49]
#define IO_WY IO[0x4A]
#define IO_WX IO[0x4B]
#define IO_VBK IO[0x4F]
#define IO_HDMA1 IO[0x51]
#define IO_HDMA2 IO[0x52]
#define IO_HDMA3 IO[0x53]
#define IO_HDMA4 IO[0x54]
#define IO_HDMA5 IO[0x55]
#define IO_RP IO[0x56] // (GBC) infrared
#define IO_BCPS IO[0x68]
// #define IO_BCPD IO[0x69]
#define IO_OCPS IO[0x6A]
// #define IO_OCPD IO[0x6B]
#define IO_OPRI IO[0x6C] // (GBC) object priority
// MISC
#define IO_SVBK IO[0x70]
#define IO_KEY1 IO[0x4D]
#define IO_BOOTROM IO[0x50]
#define IO_IF IO[0x0F]
#define IO_IE gb->hram[0x7F]
// undocumented registers (GBC)
#define IO_72 IO[0x72]
#define IO_73 IO[0x73]
#define IO_74 IO[0x74]
#define IO_75 IO[0x75] // only bit 4-6 are usable


enum GB_Interrupts
{
    GB_INTERRUPT_VBLANK     = 0x01,
    GB_INTERRUPT_LCD_STAT   = 0x02,
    GB_INTERRUPT_TIMER      = 0x04,
    GB_INTERRUPT_SERIAL     = 0x08,
    GB_INTERRUPT_JOYPAD     = 0x10,
};

enum GB_StatusModes
{
    STATUS_MODE_HBLANK      = 0,
    STATUS_MODE_VBLANK      = 1,
    STATUS_MODE_SPRITE      = 2,
    STATUS_MODE_TRANSFER    = 3
};

enum GB_StatIntModes
{
    STAT_INT_MODE_0             = 0x08,
    STAT_INT_MODE_1             = 0x10,
    STAT_INT_MODE_2             = 0x20,
    STAT_INT_MODE_COINCIDENCE   = 0x40
};


// these internally discard the const when passing the gb
// struct to the error callback.
// this function is still marked const because it is often called
// in const functions.
void GB_throw_error(const struct GB_Core* gb, enum GB_ErrorDataType type, const char* message);

void GB_rtc_tick_frame(struct GB_Core* gb);

uint8_t GB_ioread(struct GB_Core* gb, uint16_t addr);
void GB_iowrite(struct GB_Core* gb, uint16_t addr, uint8_t value);
uint8_t GB_read8(struct GB_Core* gb, const uint16_t addr);
void GB_write8(struct GB_Core* gb, uint16_t addr, uint8_t value);
uint16_t GB_read16(struct GB_Core* gb, uint16_t addr);
void GB_write16(struct GB_Core* gb, uint16_t addr, uint16_t value);

void GB_on_lcdc_write(struct GB_Core* gb, const uint8_t value);

uint8_t GB_apu_ioread(const struct GB_Core* gb, uint16_t addr);
void GB_apu_iowrite(struct GB_Core* gb, uint16_t addr, uint8_t value);

uint8_t GB_serial_sb_read(const struct GB_Core* gb);
void GB_serial_sc_write(struct GB_Core* gb, const uint8_t data);

void GB_bcpd_write(struct GB_Core* gb, uint8_t value);
void GB_ocpd_write(struct GB_Core* gb, uint8_t value);

uint8_t GBC_bcpd_read(struct GB_Core* gb);
uint8_t GBC_ocpd_read(struct GB_Core* gb);

uint8_t GB_hdma5_read(const struct GB_Core* gb);
void GB_hdma5_write(struct GB_Core* gb, uint8_t value);

// these should also be static
bool GB_setup_mbc(struct GB_Cart* mbc, const struct GB_CartHeader* header);
void GB_setup_mmap(struct GB_Core* gb);
void GB_update_rom_banks(struct GB_Core* gb);
void GB_update_ram_banks(struct GB_Core* gb);
void GB_update_vram_banks(struct GB_Core* gb);
void GB_update_wram_banks(struct GB_Core* gb);

// used internally
void GB_DMA(struct GB_Core* gb);
void GB_draw_scanline(struct GB_Core* gb);
void GB_update_all_colours_gb(struct GB_Core* gb);
void GB_set_coincidence_flag(struct GB_Core* gb, const bool n);

void GB_set_status_mode(struct GB_Core* gb, const enum GB_StatusModes mode);
enum GB_StatusModes GB_get_status_mode(const struct GB_Core* gb);

void GB_compare_LYC(struct GB_Core* gb);

uint8_t GB_joypad_read(const struct GB_Core* gb);
void GB_joypad_write(struct GB_Core* gb, uint8_t value);

void GB_enable_interrupt(struct GB_Core* gb, const enum GB_Interrupts interrupt);
void GB_disable_interrupt(struct GB_Core* gb, const enum GB_Interrupts interrupt);

// used internally
uint16_t GB_cpu_run(struct GB_Core* gb, uint16_t cycles);
void GB_timer_run(struct GB_Core* gb, uint16_t cycles);
void GB_ppu_run(struct GB_Core* gb, uint16_t cycles);
void GB_apu_run(struct GB_Core* gb, uint16_t cycles);

bool GB_is_lcd_enabled(const struct GB_Core* gb);
bool GB_is_win_enabled(const struct GB_Core* gb);
bool GB_is_obj_enabled(const struct GB_Core* gb);
bool GB_is_bg_enabled(const struct GB_Core* gb);


// SGB stuff
bool SGB_handle_joyp_read(const struct GB_Core* gb, uint8_t* data_out);
void SGB_handle_joyp_write(struct GB_Core* gb, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // _GB_INTERNAL_H_
