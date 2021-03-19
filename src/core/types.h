#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "tables/palette_table.h"

#include <stddef.h>

#define GB_LITTLE_ENDIAN 1
#define GB_BIG_ENDIAN 2

// todo: add more compilers endianess detection.
#ifndef GB_ENDIAN
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define GB_ENDIAN GB_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define GB_ENDIAN GB_BIG_ENDIAN
#endif
#endif /* GB_ENDIAN */

#if defined GB_NO_STDLIB && !defined GB_CUSTOM_ALLOC
#define GB_NO_DYNAMIC_MEMORY
#endif /* GB_NO_STDLIB && GB_CUSTOM_ALLOC */

#ifndef GB_ENDIAN
#error GB_ENDIAN IS NOT SET! UNABLE TO DEDUCE PLATFORM ENDIANESS
#endif /* GB_ENDIAN */

#include <stdint.h>
typedef uint8_t GB_U8;
typedef uint16_t GB_U16;
typedef uint32_t GB_U32;
typedef int8_t GB_S8;
typedef int16_t GB_S16;
typedef int32_t GB_S32;

#define GB_TRUE 1
#define GB_FALSE 0
typedef GB_U8 GB_BOOL;

// fwd declare structs (will split into sep .c/.h soon)
struct GB_Data;
struct GB_MbcData;
struct GB_Rtc;
struct GB_Sprite;
struct GB_CartHeader;
struct GB_IFile;
struct GB_Joypad;

#define GB_SCREEN_WIDTH 160
#define GB_SCREEN_HEIGHT 144

#define GB_BOOTROM_SIZE 0x100

#define GB_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define GB_MAX(x, y) (((x) > (y)) ? (x) : (y))

// todo: prefix with GB
// TODO: detect if using gcc compiler with macros
#define GCC_OPTIMISATIONS
#ifdef GCC_OPTIMISATIONS
#define LIKELY(c) (__builtin_expect(c,1))
#define UNLIKELY(c) (__builtin_expect(c,0))
#else
#define LIKELY(c) ((c))
#define UNLIKELY(c) ((c))
#endif

// todo: prefix with GB
#define UNUSED(var) ((void)(var))

#define GB_ARR_SIZE(array) (sizeof(array) / sizeof(array[0]))

// todo: prefix with GB
#define IO gb->io
// JOYPAD
#define IO_JYP IO[0x00]
// SERIAL
#define IO_SB IO[0x01]
#define IO_SC IO[0x02]
// TIMERS
#define IO_DIV16 *(GB_U16*)(IO + 0x03)
#define IO_DIV IO[0x04]
#define IO_TIMA IO[0x05]
#define IO_TMA IO[0x06]
#define IO_TAC IO[0x07]
// APU
#define IO_NR10 IO[0x10]
#define IO_NR11 IO[0x11]
#define IO_NR12 IO[0x12]
#define IO_NR13 IO[0x13]
#define IO_NR14 IO[0x14]
#define IO_NR21 IO[0x16]
#define IO_NR22 IO[0x17]
#define IO_NR23 IO[0x18]
#define IO_NR24 IO[0x19]
#define IO_NR30 IO[0x1A]
#define IO_NR31 IO[0x1B]
#define IO_NR32 IO[0x1C]
#define IO_NR33 IO[0x1D]
#define IO_NR34 IO[0x1E]
#define IO_NR41 IO[0x20]
#define IO_NR42 IO[0x21]
#define IO_NR43 IO[0x22]
#define IO_NR44 IO[0x23]
#define IO_NR50 IO[0x24]
#define IO_NR51 IO[0x25]
#define IO_NR52 IO[0x26]
#define IO_WAVE_TABLE (IO+0x30)
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
#define IO_BCPS IO[0x68]
#define IO_OCPS IO[0x69]
// MISC
#define IO_SVBK IO[0x70]
#define IO_KEY1 IO[0x4D]
#define IO_BOOTROM IO[0x50]
#define IO_IF IO[0x0F]
#define IO_IE gb->hram[0x7F]

enum GB_MbcType {
	MBC_TYPE_0,
	MBC_TYPE_1,
	MBC_TYPE_NONE = 0xFF,
};

// for changing the internal colour buffer.
// this is is faster than converting each colour to rgb when rendering.
// most consoles support rgb565 or bg565 but not bgr555 (default).
// changing it GB side means that it potentially only calculates the colours
// once, when updating the palette.
// for built-in GB colours, i can have 4 precalculated colour tables,
// so the only overhead is a switch statement when updating the palettes,
// this can also be removed by using a func ptr, but no real need honestly.
enum GB_ColourMode {
	GB_COLOUR_BGR555,
	GB_COLOUR_BGR565,
	GB_COLOUR_RGB555,
	GB_COLOUR_RGB565,
};

enum GB_SaveSizes {
	GB_SAVE_SIZE_NONE = 0,
	GB_SAVE_SIZE_1 = 0x800U,
	GB_SAVE_SIZE_2 = 0x2000U,
	GB_SAVE_SIZE_3 = 0x8000U,
	GB_SAVE_SIZE_4 = 0x20000U,
	GB_SAVE_SIZE_5 = 0x10000U,

	GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_5,
};

enum GB_MbcFlags {
	MBC_FLAGS_NONE = 0,
	MBC_FLAGS_RAM = 1 << 0,
	MBC_FLAGS_BATTERY = 1 << 1,
	MBC_FLAGS_RTC = 1 << 2,
	// not used yet
	MBC_FLAGS_RUMBLE = 1 << 3,
};

enum GB_CpuFlags {
	GB_CPU_FLAG_C,
	GB_CPU_FLAG_H,
	GB_CPU_FLAG_N,
	GB_CPU_FLAG_Z
};

enum GB_CpuRegisters {
	GB_CPU_REGISTER_B,
	GB_CPU_REGISTER_C,
	GB_CPU_REGISTER_D,
	GB_CPU_REGISTER_E,
	GB_CPU_REGISTER_H,
	GB_CPU_REGISTER_L,
	GB_CPU_REGISTER_A,
	GB_CPU_REGISTER_F
};

enum GB_CpuRegisterPairs {
	GB_CPU_REGISTER_PAIR_BC,
	GB_CPU_REGISTER_PAIR_DE,
	GB_CPU_REGISTER_PAIR_HL,
	GB_CPU_REGISTER_PAIR_AF,
	GB_CPU_REGISTER_PAIR_SP,
	GB_CPU_REGISTER_PAIR_PC
};

enum GB_Button {
	GB_BUTTON_A = 1 << 0,
    GB_BUTTON_B = 1 << 1,
    GB_BUTTON_SELECT = 1 << 2,
    GB_BUTTON_START = 1 << 3,

    GB_BUTTON_RIGHT = 1 << 4,
    GB_BUTTON_LEFT = 1 << 5,
    GB_BUTTON_UP = 1 << 6,
    GB_BUTTON_DOWN = 1 << 7
};

typedef void(*GB_vsync_callback_t)();
typedef void(*GB_hblank_callback_t)();
typedef void(*GB_dma_callback_t)();

// structs

enum GB_LoadSystemType {
	GB_LOAD_SYSTEM_TYPE_DMG,
	GB_LOAD_SYSTEM_TYPE_COLOUR_PLUS,
	GB_LOAD_SYSTEM_TYPE_COLOUR,
	GB_LOAD_SYSTEM_TYPE_ANY,
};

enum GB_PaletteOption {
	GB_PALETTE_OPTION_HASH,
	GB_PALETTE_OPTION_NO_OVERRIDE,
};

struct GB_Config {
	enum GB_LoadSystemType load_system_type; /* GB_LOAD_SYSTEM_TYPE_ANY */
	enum GB_PaletteOption palette_option;
};

struct GB_CartHeader {
	GB_U8 entry_point[0x4];
	GB_U8 logo[0x30];
	char title[0x10];
	GB_U16 new_licensee_code;
	GB_U8 sgb_flag;
	GB_U8 cart_type;
	GB_U8 rom_size;
	GB_U8 ram_size;
	GB_U8 destination_code;
	GB_U8 old_licensee_code;
	GB_U8 rom_version;
	GB_U8 header_checksum;
	GB_U16 global_checksum;
};

struct GB_RomInfo {
	char name[0x10]; /* from cart_header */
	GB_U32 rom_size;
	GB_U32 ram_size;
	GB_U8 mbc_flags; /* flags ored together */
};

// todo: ensure packed to 2 bytes (GB_U16).
struct GB_Colour {
#if GB_ENDIAN == GB_LITTLE_ENDIAN
	GB_U8 r:5;
	GB_U8 g:5;
	GB_U8 b:5;
	GB_U8 _pad[1];
#else
	GB_U8 _pad[1];
	GB_U8 b:5;
	GB_U8 g:5;
	GB_U8 r:5;
#endif /* GB_ENDIAN */
};

struct GB_BgAttributes {
#if GB_ENDIAN == GB_LITTLE_ENDIAN
	GB_U8 pal:3;
	GB_U8 bank:1;
	GB_U8 _pad[1];
	GB_U8 xflip:1;
	GB_U8 yflip:1;
	GB_U8 priority:1;
#else
	GB_U8 priority:1;
	GB_U8 yflip:1;
	GB_U8 xflip:1;
	GB_U8 _pad[1];
	GB_U8 bank:1;
	GB_U8 pal:3;
#endif /* GB_ENDIAN */
};

// todo: ensure packed to 4 bytes.
struct GB_Sprite {
	GB_U8 y;
	GB_U8 x;
	GB_U8 num;
#if GB_ENDIAN == GB_LITTLE_ENDIAN
	struct {
		GB_U8 pal_gbc:3;
		GB_U8 bank:1;
		GB_U8 pal_gb:1;
		GB_U8 xflip:1;
		GB_U8 yflip:1;
		GB_U8 priority:1;
	} flag;
#else
	struct {
		GB_U8 priority:1;
		GB_U8 yflip:1;
		GB_U8 xflip:1;
		GB_U8 pal_gb:1;
		GB_U8 bank:1;
		GB_U8 pal_gbc:3;
	} flag;
#endif /* GB_ENDIAN */
};

struct GB_Rtc {
	GB_U8 S;
	GB_U8 M;
	GB_U8 H;
	GB_U8 DL;
	GB_U8 DH;
};

struct GB_Joypad {
	GB_U8 var;
};

struct GB_Cpu {
	GB_U16 cycles;
	GB_U16 SP;
	GB_U16 PC;
	GB_U8 registers[8];
	GB_BOOL ime;
	GB_BOOL halt;
};

struct GB_Ppu {
	GB_S16 next_cycles;
	GB_U16 pixles[GB_SCREEN_HEIGHT][GB_SCREEN_WIDTH]; // h*w
	union {
		struct GB_BgAttributes bg_attributes[2][0x2000];
		GB_U8 vram[2][0x2000]; // 2 banks of 8kb
	};
	union {
		struct GB_Sprite sprites[40]; // will remove if unions cause problems.
		GB_U8 oam[0xA0]; // sprite mem
	};
	union {
		struct GB_Colour bg_colours_bgr555[8][4];
    	GB_U16 bg_colours[8][4]; // calculate the colours from the palette once.
	};
	union {
		struct GB_Colour obj_colours_bgr555[8][4];
    	GB_U16 obj_colours[8][4];
	};
	GB_U8 line_counter;
	GB_U8 bg_palette[0x40]; // background palette memory.
	GB_U8 obj_palette[0x40]; // sprite palette memory.
	GB_BOOL dirty_bg[8]; // only update the colours if the palette changes values.
    GB_BOOL dirty_obj[8];
};

// todo: remove mbc struct into here.
struct GB_Cart {
	void (*write)(struct GB_Data* gb, GB_U16 addr, GB_U8 val);
	const GB_U8* (*get_rom_bank)(struct GB_Data* gb, GB_U8 val);
	const GB_U8* (*get_ram_bank)(struct GB_Data* gb, GB_U8 val);

	GB_U8* rom;
	GB_U8 ram[0x10000];
	GB_U32 rom_size;
	GB_U32 ram_size;
	GB_U16 rom_bank;
	GB_U16 ram_bank;
	GB_U8 flags;
	struct GB_Rtc rtc;
	GB_BOOL bank_mode;
	GB_BOOL ram_enabled;
	GB_BOOL in_ram;

	enum GB_MbcType mbc_type;
};

struct GB_Timer {
	GB_S16 next_cycles;
};

struct GB_Data {
	const GB_U8* mmap[0x10];
	GB_U8 io[0x80]; // easier to have io as an array than individual bytes
	GB_U8 hram[0x80]; // 0x7F + IE reg
	GB_U8 wram[8][0x1000]; // extra 6 banks in gbc
	struct GB_Cpu cpu;
	struct GB_Ppu ppu;
	struct GB_Cart cart;
	struct GB_Timer timer;
	struct GB_Joypad joypad;

	struct GB_PaletteEntry palette; /* default */

	GB_vsync_callback_t vsync_cb;
	GB_hblank_callback_t hblank_cb;

	void (*rom_free_func)(void*);
};

// i decided that the ram usage / statefile size is less important
// than code complexity, reliablility and speed.
// because of this, even if the game has no ram (tetris), it will
// still take up 64k.
// however *most* games do have ram, and fixing the size removes the
// need to allocate and simplifies savestate / rewinding *a lot*.
struct GB_CartState {
	GB_U16 rom_bank;
	GB_U16 ram_bank;
	GB_U8 ram[0x10000];
	struct GB_Rtc rtc;
	GB_BOOL bank_mode;
	GB_BOOL ram_enabled;
	GB_BOOL in_ram;
};

struct GB_CoreState {
	GB_U8 io[0x80]; // easier to have io as an array than individual bytes
	GB_U8 hram[0x80]; // 0x7F + IE reg
	GB_U8 wram[8][0x1000]; // extra 6 banks in gbc
	struct GB_Cpu cpu;
	struct GB_Ppu ppu;
	struct GB_Timer timer;
	struct GB_CartState cart;
};

struct GB_StateHeader {
	GB_U32 magic;
    GB_U32 version;
    GB_U8 padding[0x20];
};

struct GB_State {
	struct GB_StateHeader header;
	struct GB_CoreState core;
};

struct GB_SaveData {
	GB_U32 size; // is filled internally
	GB_U8 data[GB_SAVE_SIZE_MAX];
	struct GB_Rtc rtc;
	GB_BOOL has_rtc; // is filled internally
};

struct GB_CartName {
	char name[0x10]; // this is NULL terminated
};

#ifdef __cplusplus
}
#endif