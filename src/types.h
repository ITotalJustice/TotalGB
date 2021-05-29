#ifndef _GB_TYPES_H_
#define _GB_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_LIB
    #define GBAPI __declspec(dllexport)
  #else
    #define GBAPI __declspec(dllimport)
  #endif
#else
  #ifdef BUILDING_LIB
      #define GBAPI __attribute__ ((visibility ("default")))
  #else
      #define GBAPI
  #endif
#endif


#include "tables/palette_table.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


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

#ifndef GB_ENDIAN
#error GB_ENDIAN IS NOT SET! UNABLE TO DEDUCE PLATFORM ENDIANESS
#endif /* GB_ENDIAN */


// fwd declare structs
struct GB_Core;
struct GB_Rtc;
struct GB_Sprite;
struct GB_CartHeader;
struct GB_Joypad;
struct GB_ApuCallbackData;
struct GB_Config;
struct GB_ErrorData;


enum {
    GB_SCREEN_WIDTH = 160,
    GB_SCREEN_HEIGHT = 144,

    GB_BOOTROM_SIZE = 0x100,

    GB_FRAME_CPU_CYCLES = 70224,
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
    GB_SAVE_SIZE_NONE   = 0,
    GB_SAVE_SIZE_1      = 0x800,
    GB_SAVE_SIZE_2      = 0x2000,
    GB_SAVE_SIZE_3      = 0x8000,
    GB_SAVE_SIZE_4      = 0x20000,
    GB_SAVE_SIZE_5      = 0x10000,

#ifdef GB_MAX_SRAM_SIZE
    #if GB_MAX_SRAM_SIZE == 0
        GB_SAVE_SIZE_MAX = 1, // this is so that the ram arrays don't error!
    #elif GB_MAX_SRAM_SIZE == 1
        GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_1
    #elif GB_MAX_SRAM_SIZE == 2
        GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_2
    #elif GB_MAX_SRAM_SIZE == 3
        GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_3
    #elif GB_MAX_SRAM_SIZE == 4
        GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_4
    #elif GB_MAX_SRAM_SIZE == 5
        GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_5
    #else
        #error "Invalid SRAM size set!, Valid range is 0-5"
    #endif
#else
    GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_5,
#endif
};

enum GB_MbcFlags {
    MBC_FLAGS_NONE      = 0,
    MBC_FLAGS_RAM       = 1 << 0,
    MBC_FLAGS_BATTERY   = 1 << 1,
    MBC_FLAGS_RTC       = 1 << 2,
    MBC_FLAGS_RUMBLE    = 1 << 3,
};

enum GB_RtcMappedReg {
    GB_RTC_MAPPED_REG_S,
    GB_RTC_MAPPED_REG_M,
    GB_RTC_MAPPED_REG_H,
    GB_RTC_MAPPED_REG_DL,
    GB_RTC_MAPPED_REG_DH,
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
    GB_BUTTON_A         = 1 << 0,
    GB_BUTTON_B         = 1 << 1,
    GB_BUTTON_SELECT    = 1 << 2,
    GB_BUTTON_START     = 1 << 3,

    GB_BUTTON_RIGHT     = 1 << 4,
    GB_BUTTON_LEFT      = 1 << 5,
    GB_BUTTON_UP        = 1 << 6,
    GB_BUTTON_DOWN      = 1 << 7,

    // helpers
    GB_BUTTON_XAXIS = GB_BUTTON_RIGHT | GB_BUTTON_LEFT,
    GB_BUTTON_YAXIS = GB_BUTTON_UP | GB_BUTTON_DOWN,
    GB_BUTTON_DIRECTIONAL = GB_BUTTON_XAXIS | GB_BUTTON_YAXIS,
};

enum GB_Interrupts {
    GB_INTERRUPT_VBLANK     = 0x01,
    GB_INTERRUPT_LCD_STAT   = 0x02,
    GB_INTERRUPT_TIMER      = 0x04,
    GB_INTERRUPT_SERIAL     = 0x08,
    GB_INTERRUPT_JOYPAD     = 0x10,
};

enum GB_StatusModes {
    STATUS_MODE_HBLANK      = 0,
    STATUS_MODE_VBLANK      = 1,
    STATUS_MODE_SPRITE      = 2,
    STATUS_MODE_TRANSFER    = 3
};

enum GB_StatIntModes {
    STAT_INT_MODE_0             = 0x08,
    STAT_INT_MODE_1             = 0x10,
    STAT_INT_MODE_2             = 0x20,
    STAT_INT_MODE_COINCIDENCE   = 0x40
};

// the system type is set based on the game that is loaded
// for example, if a gbc ONLY game is loaded, the system type is
// set to GBC.
// TODO: support manually overriding the system type for supported roms
// for example, forcing DMG type for games that support both DMG and GBC.
// for this, it's probably best to return a custom error if the loaded rom
// is valid, but ONLY suppports GBC type, but DMG was forced...
enum GB_SystemType {
    GB_SYSTEM_TYPE_DMG,
    GB_SYSTEM_TYPE_SGB,
    GB_SYSTEM_TYPE_GBC
};

// this setting only applies when the game is loaded as DMG or SGB game.
// GBC games set their own colour palette and cannot be customised!
enum GB_PaletteConfig {
    // uses the default palette
    GB_PALETTE_CONFIG_NONE          = 0,
    // these can be OR'd together to set an additional option.
    // if both are OR'd, first, the game will try to use a builtin palette.
    // if a builtin palette cannot be found, then it will fallback to the
    // custom palette instead.
    GB_PALETTE_CONFIG_USE_CUSTOM    = 1 << 0,
    GB_PALETTE_CONFIG_USE_BUILTIN   = 1 << 1,
};

enum GB_SystemTypeConfig {
    GB_SYSTEM_TYPE_CONFIG_NONE = 0,
    // DMG and SGB will result is a romload error
    // if the rom ONLY supports GBC system type
    GB_SYSTEM_TYPE_CONFIG_DMG,
    GB_SYSTEM_TYPE_CONFIG_SGB,
    GB_SYSTEM_TYPE_CONFIG_GBC
};

enum GB_RenderLayerConfig {
    GB_RENDER_LAYER_CONFIG_ALL  = 0,
    // only render part of the screen.
    // bitwise OR these together to enable multiple
    GB_RENDER_LAYER_CONFIG_BG   = 1 << 0,
    GB_RENDER_LAYER_CONFIG_WIN  = 1 << 1,
    GB_RENDER_LAYER_CONFIG_OBJ  = 1 << 2,
};

enum GB_RtcUpdateConfig {
    // the RTC is ticked at the end of each frame, once it reaches
    // 60, the counter is reset, then rtc.s is incremented.
    GB_RTC_UPDATE_CONFIG_FRAME,
    // calls time() then localtime()
    // then parses the tm struct and sets the RTC.
    GB_RTC_UPDATE_CONFIG_USE_LOCAL_TIME,
    // the RTC will not be updated (ticked) on the core side.
    // this is useful for if you want to set the RTC to system time
    GB_RTC_UPDATE_CONFIG_NONE,
};

struct GB_Config {
    enum GB_PaletteConfig palette_config;
    struct GB_PaletteEntry custom_palette;
    enum GB_SystemTypeConfig system_type_config;
    enum GB_RenderLayerConfig render_layer_config;
    enum GB_RtcUpdateConfig rtc_update_config;
};

enum GB_ErrorType {
    GB_ERROR_TYPE_UNKNOWN_INSTRUCTION,
    GB_ERROR_TYPE_INFO,
    GB_ERROR_TYPE_WARN,
    GB_ERROR_TYPE_ERROR,
    GB_ERROR_TYPE_UNK
};

struct GB_UnkownInstructionTypeData {
    uint8_t opcode;
    bool cb_prefix;
};

enum GB_ErrorDataType {
    GB_ERROR_DATA_TYPE_UNK,
    GB_ERROR_DATA_TYPE_NULL_PARAM,
    GB_ERROR_DATA_TYPE_ROM,
    GB_ERROR_DATA_TYPE_SRAM,
    GB_ERROR_DATA_TYPE_SAVE,
};

struct GB_InfoTypeData {
    char message[0x200]; // NULL terminated string
};

struct GB_WarnTypeData {
    char message[0x200]; // NULL terminated string
};

struct GB_ErrorTypeData {
    enum GB_ErrorDataType type;
    char message[0x200]; // NULL terminated string
};

struct GB_ErrorData {
    enum GB_ErrorType type;
    union {
        struct GB_UnkownInstructionTypeData unk_instruction;
        struct GB_InfoTypeData info;
        struct GB_WarnTypeData warn;
        struct GB_ErrorTypeData error;
    } data;
};

// user-set callbacks
typedef void(*GB_apu_callback_t)(struct GB_Core* gb, void* user,
    const struct GB_ApuCallbackData* data
);

typedef void(*GB_error_callback_t)(struct GB_Core* gb, void* user,
    struct GB_ErrorData* e
);

typedef void(*GB_vblank_callback_t)(struct GB_Core* gb, void* user);
typedef void(*GB_hblank_callback_t)(struct GB_Core* gb, void* user);
typedef void(*GB_dma_callback_t)(struct GB_Core* gb, void* user);
typedef void(*GB_halt_callback_t)(struct GB_Core* gb, void* user);
typedef void(*GB_stop_callback_t)(struct GB_Core* gb, void* user);


enum GB_AudioCallbackMode {
    /* sample buffer is first filled, then callback is called */
    AUDIO_CALLBACK_FILL_SAMPLES,
    /* this will push 1 sample at a time, as fast as possible */
    AUDIO_CALLBACK_PUSH_ALL,
};

struct GB_AudioCallbackData {
    GB_apu_callback_t cb;
    void* user_data;
    enum GB_AudioCallbackMode mode;
    int freq;
};

enum GB_LinkTransferType {
    // tells the other gameboy that it is NOT
    // the master clock, instead it is the slave
    GB_LINK_TRANSFER_TYPE_SLAVE_SET,
    // normal data transfer, sent from the master to the slave.
    GB_LINK_TRANSFER_TYPE_DATA,
};

struct GB_LinkCableData {
    enum GB_LinkTransferType type;
    uint8_t in_data;
    uint8_t out_data;
};

// serial data transfer callback is used when IO_SC is set to 0x81
// at which point, this callback is called, sending the data in
// IO_SB, this will be data_in.
// if the recieving GB is accepting data, then it should set data_out
// equal to it's IO_SB, then, clear bit-7 of it's IO_SC, finally,
// push a serial_interrupt.

// this callback should return GB_TRUE if the data was accepted AND
// data_out is filled, else, return GB_FALSE
typedef bool(*GB_serial_transfer_t)(void* user, struct GB_LinkCableData* data);

// structs

struct GB_CartHeader {
    uint8_t entry_point[0x4];
    uint8_t logo[0x30];
    char title[0x10];
    uint16_t new_licensee_code;
    uint8_t sgb_flag;
    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t rom_version;
    uint8_t header_checksum;
    uint16_t global_checksum;
};

struct GB_RomInfo {
    char name[0x10]; /* from cart_header */
    uint32_t rom_size;
    uint32_t ram_size;
    uint8_t mbc_flags; /* flags ored together */
};

struct GB_BgAttributes {
#if GB_ENDIAN == GB_LITTLE_ENDIAN
    uint8_t pal:3;
    uint8_t bank:1;
    uint8_t _pad:1;
    uint8_t xflip:1;
    uint8_t yflip:1;
    uint8_t priority:1;
#else
    uint8_t priority:1;
    uint8_t yflip:1;
    uint8_t xflip:1;
    uint8_t _pad[1];
    uint8_t bank:1;
    uint8_t pal:3;
#endif /* GB_ENDIAN */
};

// todo: ensure packed to 4 bytes.
struct GB_Sprite {
    uint8_t y;
    uint8_t x;
    uint8_t num;
#if GB_ENDIAN == GB_LITTLE_ENDIAN
    struct {
        uint8_t pal_gbc:3;
        uint8_t bank:1;
        uint8_t pal_gb:1;
        uint8_t xflip:1;
        uint8_t yflip:1;
        uint8_t priority:1;
    } flag;
#else
    struct {
        uint8_t priority:1;
        uint8_t yflip:1;
        uint8_t xflip:1;
        uint8_t pal_gb:1;
        uint8_t bank:1;
        uint8_t pal_gbc:3;
    } flag;
#endif /* GB_ENDIAN */
};

struct GB_Rtc {
    uint8_t S;
    uint8_t M;
    uint8_t H;
    uint8_t DL;
    uint8_t DH;
};

struct GB_Joypad {
    uint8_t var;
};

struct GB_Cpu {
    uint16_t cycles;
    uint16_t SP;
    uint16_t PC;
    uint8_t registers[8];

    bool c;
    bool h;
    bool n;
    bool z;

    bool ime;
    bool halt;
    bool double_speed;
};

struct GB_Ppu {
    int16_t next_cycles;

    uint8_t vram[2][0x2000]; // 2 banks of 8kb
    uint8_t oam[0xA0]; // sprite mem

    uint32_t bg_colours[8][4]; // calculate the colours from the palette once.
    uint32_t obj_colours[8][4];

    // these are set when a hdma occurs (not a DMA or GDMA)
    uint16_t hdma_src_addr;
    uint16_t hdma_dst_addr;
    uint16_t hdma_length;

    // this is the internal line counter which is used as the index
    // for the window instead of IO_LY.
    uint8_t window_line;

    uint8_t bg_palette[0x40]; // background palette memory.
    uint8_t obj_palette[0x40]; // sprite palette memory.

    bool dirty_bg[8]; // only update the colours if the palette changes values.
    bool dirty_obj[8];
};

// todo: fix bad name
struct GB_MemMapEntry {
    const uint8_t* ptr;
    uint16_t mask;
};

struct MBC_RomBankInfo {
    struct GB_MemMapEntry entries[4];
};

struct MBC_RamBankInfo {
    struct GB_MemMapEntry entries[2];
};

// todo: remove all bitfields, there's no reason to be using them!
struct GB_Cart {
    void (*write)(struct GB_Core* gb, uint16_t addr, uint8_t val);

    struct MBC_RomBankInfo (*get_rom_bank)(struct GB_Core* gb, uint8_t bank);
    struct MBC_RamBankInfo (*get_ram_bank)(struct GB_Core* gb);

    const uint8_t* rom;

    uint8_t ram[GB_SAVE_SIZE_MAX];
    uint32_t rom_size;
    uint32_t ram_size;

    uint16_t rom_bank_max;
    uint16_t rom_bank;
    uint8_t rom_bank_lo;
    uint8_t rom_bank_hi;

    uint8_t ram_bank_max;
    uint8_t ram_bank;

    enum GB_RtcMappedReg rtc_mapped_reg;
    struct GB_Rtc rtc;
    uint8_t internal_rtc_counter;

    bool bank_mode;
    bool ram_enabled;
    bool in_ram;

    uint8_t flags;
};

struct GB_Timer {
    int16_t next_cycles;
};

// TODO: REMOVE ALL BITFIELDS!
struct GB_ApuSquare1 {
    struct {
        uint8_t sweep_period : 3;
        uint8_t negate : 1;
        uint8_t shift : 3;
    } nr10;

    struct {
        uint8_t duty : 2;
        uint8_t length_load : 6;
    } nr11;

    struct {
        uint8_t starting_vol : 4;
        uint8_t env_add_mode : 1;
        uint8_t period : 3;
    } nr12;

    struct {
        uint8_t freq_lsb : 8;
    } nr13;

    struct {
        uint8_t trigger : 1;
        uint8_t length_enable : 1;
        uint8_t freq_msb : 3;
    } nr14;

    uint16_t freq_shadow_register;
    uint8_t internal_enable_flag;

    int16_t timer;
    int8_t volume_timer;
    uint8_t duty_index : 3;
    uint8_t volume : 4;
    bool disable_env : 1;

    int8_t sweep_timer;

    uint8_t length_counter;
};

struct GB_ApuSquare2 {
    struct {
        uint8_t duty : 2;
        uint8_t length_load : 6;
    } nr21;

    struct {
        uint8_t starting_vol : 4;
        uint8_t env_add_mode : 1;
        uint8_t period : 3;
    } nr22;

    struct {
        uint8_t freq_lsb : 8;
    } nr23;

    struct {
        uint8_t trigger : 1;
        uint8_t length_enable : 1;
        uint8_t freq_msb : 3;
    } nr24;

    int16_t timer;
    int8_t volume_timer;
    uint8_t duty_index : 3;
    uint8_t volume : 4;
    bool disable_env : 1;

    uint8_t length_counter;
};

struct GB_ApuWave {
    struct {
        uint8_t DAC_power : 1;
    } nr30;

    struct {
        uint8_t length_load : 8;
    } nr31;

    struct {
        uint8_t vol_code : 2;
    } nr32;

    struct {
        uint8_t freq_lsb : 8;
    } nr33;

    struct {
        uint8_t trigger : 1;
        uint8_t length_enable : 1;
        uint8_t freq_msb : 3;
    } nr34;

    uint8_t wave_ram[0x10];

    uint16_t length_counter : 9;
    uint8_t sample_buffer;
    uint8_t position_counter : 6;

    int16_t timer;
};

struct GB_ApuNoise {
    struct {
        uint8_t length_load : 6;
    } nr41;

    struct {
        uint8_t starting_vol : 4;
        uint8_t env_add_mode : 1;
        uint8_t period : 3;
    } nr42;

    struct {
        uint8_t clock_shift : 4;
        uint8_t width_mode : 1;
        uint8_t divisor_code : 3;
    } nr43;

    struct {
        uint8_t trigger : 1;
        uint8_t length_enable : 1;
    } nr44;

    int32_t timer;

    uint16_t LFSR : 15;

    int8_t volume_timer;
    uint8_t volume : 4;
    bool disable_env : 1;

    uint8_t length_counter;
};

struct GB_ApuControl {
    struct {
        uint8_t vin_l : 1;
        uint8_t left_vol : 3;
        uint8_t vin_r : 1;
        uint8_t right_vol : 3;
    } nr50;

    struct {
        uint8_t noise_left : 1;
        uint8_t wave_left : 1;
        uint8_t square2_left : 1;
        uint8_t square1_left : 1;

        uint8_t noise_right : 1;
        uint8_t wave_right : 1;
        uint8_t square2_right : 1;
        uint8_t square1_right : 1;
    } nr51;

    struct {
        uint8_t power : 1;
        uint8_t noise : 1;
        uint8_t wave : 1;
        uint8_t square2 : 1;
        uint8_t square1 : 1;
    } nr52;
};

struct GB_ApuCallbackData {
    // type
    union {
        struct {
            // 2 because of stereo
            int8_t ch1[2];
            int8_t ch2[2];
            int8_t ch3[2];
            int8_t ch4[2];
        } samples;

        struct {
            // 512 samples * sterero
            int8_t ch1[512 * 2];
            int8_t ch2[512 * 2];
            int8_t ch3[512 * 2];
            int8_t ch4[512 * 2];
        } buffers;
    } data;
};

struct GB_Apu {
    uint16_t next_frame_sequencer_cycles;
    uint16_t next_sample_cycles;

    struct GB_ApuSquare1 square1;
    struct GB_ApuSquare2 square2;
    struct GB_ApuWave wave;
    struct GB_ApuNoise noise;
    struct GB_ApuControl control;

    uint8_t frame_sequencer_counter : 3;

    struct GB_ApuCallbackData samples;
    uint16_t samples_count;
    enum GB_AudioCallbackMode sample_mode;
};

// TODO: this struct needs to be re-organised.
// atm, i've just been dumping vars in here as one big container,
// which works fine, though, it's starting to get messy, and could be
// *slightly* optimised by putting common accessed vars next to each other.
struct GB_Core {
    struct GB_MemMapEntry mmap[0x10];

    uint8_t io[0x80]; // easier to have io as an array than individual bytes
    uint8_t hram[0x80]; // 0x7F + IE reg
    uint8_t wram[8][0x1000]; // extra 6 banks in gbc

    struct GB_Cpu cpu;
    struct GB_Ppu ppu;
    struct GB_Apu apu;
    struct GB_Cart cart;
    struct GB_Timer timer;
    struct GB_Joypad joypad;

    bool skip_next_frame;

    struct GB_PaletteEntry palette; /* default */

    enum GB_SystemType system_type; // set by the rom itself

    struct GB_Config config;

    void* pixels;
    uint32_t pitch;

    // TODO: only have 1 user data ptr, not per callback!!!
    GB_apu_callback_t apu_cb;
    void* apu_cb_user_data;

    GB_serial_transfer_t link_cable;
    void* link_cable_user_data;
    bool is_master;

    // callbacks + user_data
    GB_vblank_callback_t vblank_cb;
    void* vblank_cb_user_data;

    GB_hblank_callback_t hblank_cb;
    void* hblank_cb_user_data;

    GB_dma_callback_t dma_cb;
    void* dma_cb_user_data;

    GB_halt_callback_t halt_cb;
    void* halt_cb_user_data;

    GB_stop_callback_t stop_cb;
    void* stop_cb_user_data;

    GB_error_callback_t error_cb;
    void* error_cb_user_data;
};

// i decided that the ram usage / statefile size is less important
// than code complexity, reliablility and speed.
// because of this, even if the game has no ram (tetris), it will
// still take up 64k.
// however *most* games do have ram, and fixing the size removes the
// need to allocate and simplifies savestate / rewinding *a lot*.
struct GB_CartState {
    uint8_t ram[GB_SAVE_SIZE_MAX];

    uint16_t rom_bank;
    uint8_t rom_bank_lo;
    uint8_t rom_bank_hi;

    uint8_t ram_bank;

    enum GB_RtcMappedReg rtc_mapped_reg;
    struct GB_Rtc rtc;
    uint8_t internal_rtc_counter;

    bool bank_mode;
    bool ram_enabled;
    bool in_ram;
};

struct GB_CoreState {
    uint8_t io[0x80]; // easier to have io as an array than individual bytes
    uint8_t hram[0x80]; // 0x7F + IE reg
    uint8_t wram[8][0x1000]; // extra 6 banks in gbc
    struct GB_Cpu cpu;
    struct GB_Ppu ppu;
    struct GB_Apu apu;
    struct GB_Timer timer;
    struct GB_Joypad joypad;
    struct GB_CartState cart;
};

struct GB_StateHeader {
    uint32_t magic;
    uint32_t version;
    uint8_t padding[0x20];
};

struct GB_State {
    struct GB_StateHeader header;
    struct GB_CoreState core;
};

struct GB_SaveData {
    uint32_t size; // is filled internally
    uint8_t data[GB_SAVE_SIZE_MAX];
    struct GB_Rtc rtc;
    bool has_rtc : 1; // is filled internally
};

struct GB_CartName {
    char name[0x11]; // this is NULL terminated
};

#ifdef __cplusplus
}
#endif

#endif // _GB_TYPES_H_
