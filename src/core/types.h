#ifndef GB_TYPES_H
#define GB_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GB_DEBUG
    #define GB_DEBUG 0
#endif

#ifndef GBC_ENABLE
    #define GBC_ENABLE 1
#endif

#ifndef SGB_ENABLE
    #define SGB_ENABLE 0
#endif

#if defined _WIN32 || defined __CYGWIN__
    #define GBAPI __declspec(dllexport)
#else
    #define GBAPI __attribute__ ((visibility ("default")))
#endif

#ifndef GB_SINGLE_FILE
    #define GB_SINGLE_FILE 0
#endif


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


// fwd declare structs
struct GB_Core;
struct GB_Rtc;
struct GB_Sprite;
struct GB_CartHeader;
struct GB_Joypad;
struct GB_ApuCallbackData;
struct GB_Config;
struct MBC_RomBankInfo;


enum
{
    GB_SCREEN_WIDTH = 160,
    GB_SCREEN_HEIGHT = 144,

    GB_ROM_SIZE_MAX = 1024 * 1024 * 4, // 4MiB

    GB_BOOTROM_SIZE = 0x100,

#if 1
    GB_CPU_CYCLES = 4213440, // 456 * 154 (clocks per line * number of lines * 60 fps)
    GB_FRAME_CPU_CYCLES = 4213440 / 60, // 70224
#else
    GB_CPU_CYCLES = 4194304,
    GB_FRAME_CPU_CYCLES = GB_CPU_CYCLES / 60,
#endif
};


// todo: remove this
enum GB_ColourMode
{
    GB_COLOUR_BGR555,
    GB_COLOUR_BGR565,
    GB_COLOUR_RGB555,
    GB_COLOUR_RGB565,
};

enum GB_SaveSizes
{
    GB_SAVE_SIZE_NONE   = 0x00000,
    GB_SAVE_SIZE_1      = 0x00800,
    GB_SAVE_SIZE_2      = 0x02000,
    GB_SAVE_SIZE_3      = 0x08000,
    GB_SAVE_SIZE_4      = 0x20000,
    GB_SAVE_SIZE_5      = 0x10000,

    GB_SAVE_SIZE_MBC2   = 0x00200,

    GB_SAVE_SIZE_MAX = GB_SAVE_SIZE_4,
};

enum GB_MbcType
{
    GB_MbcType_0 = 1,
    GB_MbcType_1,
    GB_MbcType_2,
    GB_MbcType_3,
    GB_MbcType_5,
};

enum GB_MbcFlags
{
    MBC_FLAGS_NONE      = 0,
    MBC_FLAGS_RAM       = 1 << 0,
    MBC_FLAGS_BATTERY   = 1 << 1,
    MBC_FLAGS_RTC       = 1 << 2,
    MBC_FLAGS_RUMBLE    = 1 << 3,
};

enum GB_RtcMappedReg
{
    GB_RTC_MAPPED_REG_S,
    GB_RTC_MAPPED_REG_M,
    GB_RTC_MAPPED_REG_H,
    GB_RTC_MAPPED_REG_DL,
    GB_RTC_MAPPED_REG_DH,
};

enum GB_CpuFlags
{
    GB_CPU_FLAG_C,
    GB_CPU_FLAG_H,
    GB_CPU_FLAG_N,
    GB_CPU_FLAG_Z
};

enum GB_CpuRegisters
{
    GB_CPU_REGISTER_B,
    GB_CPU_REGISTER_C,
    GB_CPU_REGISTER_D,
    GB_CPU_REGISTER_E,
    GB_CPU_REGISTER_H,
    GB_CPU_REGISTER_L,
    GB_CPU_REGISTER_A,
    GB_CPU_REGISTER_F
};

enum GB_CpuRegisterPairs
{
    GB_CPU_REGISTER_PAIR_BC,
    GB_CPU_REGISTER_PAIR_DE,
    GB_CPU_REGISTER_PAIR_HL,
    GB_CPU_REGISTER_PAIR_AF,
    GB_CPU_REGISTER_PAIR_SP,
    GB_CPU_REGISTER_PAIR_PC
};

enum GB_Button
{
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

// the system type is set based on the game that is loaded
// for example, if a gbc ONLY game is loaded, the system type is
// set to GBC.
// TODO: support manually overriding the system type for supported roms
// for example, forcing DMG type for games that support both DMG and GBC.
// for this, it's probably best to return a custom error if the loaded rom
// is valid, but ONLY suppports GBC type, but DMG was forced...
enum GB_SystemType
{
    GB_SYSTEM_TYPE_DMG = 1 << 0,
    GB_SYSTEM_TYPE_SGB = 1 << 1,
    GB_SYSTEM_TYPE_GBC = 1 << 2,
};

// this setting only applies when the game is loaded as DMG or SGB game.
// GBC games set their own colour palette and cannot be customised!
enum GB_PaletteConfig
{
    // uses the default palette
    GB_PALETTE_CONFIG_NONE          = 0,
    // these can be OR'd together to set an additional option.
    // if both are OR'd, first, the game will try to use a builtin palette.
    // if a builtin palette cannot be found, then it will fallback to the
    // custom palette instead.
    GB_PALETTE_CONFIG_USE_CUSTOM    = 1 << 0,
    GB_PALETTE_CONFIG_USE_BUILTIN   = 1 << 1,
};

// todo: remove these configs as they are poorly thoughtout
// and badly implemented!
enum GB_SystemTypeConfig
{
    GB_SYSTEM_TYPE_CONFIG_NONE = 0,
    // DMG and SGB will result is a romload error
    // if the rom ONLY supports GBC system type
    GB_SYSTEM_TYPE_CONFIG_DMG,
    GB_SYSTEM_TYPE_CONFIG_SGB,
    GB_SYSTEM_TYPE_CONFIG_GBC
};

enum GB_RenderLayerConfig
{
    GB_RENDER_LAYER_CONFIG_ALL  = 0,
    // only render part of the screen.
    // bitwise OR these together to enable multiple
    GB_RENDER_LAYER_CONFIG_BG   = 1 << 0,
    GB_RENDER_LAYER_CONFIG_WIN  = 1 << 1,
    GB_RENDER_LAYER_CONFIG_OBJ  = 1 << 2,
};

enum GB_RtcUpdateConfig
{
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

struct GB_PaletteEntry
{
    uint32_t BG[4];
    uint32_t OBJ0[4];
    uint32_t OBJ1[4];
};

struct GB_PalettePreviewShades
{
    uint32_t shade1;
    uint32_t shade2;
};

struct GB_Config
{
    enum GB_PaletteConfig palette_config;
    struct GB_PaletteEntry custom_palette;
    enum GB_SystemTypeConfig system_type_config;
    enum GB_RenderLayerConfig render_layer_config;
    enum GB_RtcUpdateConfig rtc_update_config;
};

// user-set callbacks
typedef void (*GB_apu_callback_t)(void* user, struct GB_ApuCallbackData* data);
typedef void (*GB_vblank_callback_t)(void* user);
typedef void (*GB_hblank_callback_t)(void* user);
typedef void (*GB_dma_callback_t)(void* user);
typedef void (*GB_halt_callback_t)(void* user);
typedef void (*GB_stop_callback_t)(void* user);

// if set, called whenever the bank changes.
// return true if the bank change was handled
typedef bool (*GB_rom_bank_callback_t)(void* user, struct MBC_RomBankInfo* info, enum GB_MbcType type, uint8_t bank);
// typedef bool (*GB_ram_bank_callback_t)(void* user, enum GB_MbcType type, uint8_t bank);

enum GB_ColourCallbackType
{
    GB_ColourCallbackType_DMG,
    GB_ColourCallbackType_GBC,
};

typedef uint32_t (*GB_colour_callback_t)(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b);

// read / write handles to mitm memory access
// this is useful for adding cheat devices as well as
// adding mem access watch points.

// the BIG downside is that this slows all r/w access due
// to doing if (has_handle) on every access.
// i need to test if an "if" if faster than a function call
// if it is, ill keep it as it is, if the function call is faster,
// then internally, reads will use this func ptr, until the user wants to
// set their own, then the ptr will be changed to a diff function
typedef bool (*GB_read_callback_t)(void* user, uint16_t addr, uint8_t* v);
typedef void (*GB_write_callback_t)(void* user, uint16_t addr, uint8_t* v);


struct GB_UserCallbacks
{
    GB_apu_callback_t       apu;
    GB_vblank_callback_t    vblank;
    GB_hblank_callback_t    hblank;
    GB_dma_callback_t       dma;
    GB_halt_callback_t      halt;
    GB_stop_callback_t      stop;
    GB_read_callback_t      read;
    GB_write_callback_t     write;
    GB_colour_callback_t    colour;
    GB_rom_bank_callback_t  rom_bank;

    void* user_apu;
    void* user_vblank;
    void* user_hblank;
    void* user_dma;
    void* user_halt;
    void* user_stop;
    void* user_read;
    void* user_write;
    void* user_colour;
    void* user_rom_bank;

    struct
    {
        unsigned freq_reload;
    } apu_data;
};

enum GB_LinkTransferType
{
    // tells the other gameboy that it is NOT
    // the master clock, instead it is the slave
    GB_LINK_TRANSFER_TYPE_SLAVE_SET,
    // normal data transfer, sent from the master to the slave.
    GB_LINK_TRANSFER_TYPE_DATA,
};

struct GB_LinkCableData
{
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

struct GB_CartHeader
{
    uint8_t entry_point[0x4];
    uint8_t logo[0x30];
    char title[0x10];
    uint8_t new_licensee_code[2];
    uint8_t sgb_flag;
    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t rom_version;
    uint8_t header_checksum;
    uint8_t global_checksum[2];
};

// todo:
struct GB_RomInfo
{
    uint32_t rom_size;
    uint32_t ram_size;
    uint8_t flags; /* flags ored together */
    uint8_t _padding[3];
};

struct GB_Rtc
{
    uint8_t S;
    uint8_t M;
    uint8_t H;
    uint8_t DL;
    uint8_t DH;
};

struct GB_Joypad
{
    uint8_t var;
};

struct GB_Cpu
{
    uint16_t cycles;
    uint16_t SP;
    uint16_t PC;
    uint8_t registers[8];

    bool c;
    bool h;
    bool n;
    bool z;

    bool ime;
    bool ime_delay;
    bool halt;
    bool halt_bug;
    bool double_speed;
    bool _padding;
};

struct GB_PalCache
{
    bool used;
    uint8_t pal;
};

struct GB_Ppu
{
    int16_t next_cycles;

#if GBC_ENABLE
    uint8_t vram[2][0x2000]; // 2 banks of 8kb
#else
    // this is a hack for now because the actual size is 2KiB,
    // however, my emu uses `vram[0][addr]` everywhere currently.
    // so for that to still work for now, using this will *always* work
    // still, though maybe smart enough compilers / sanitizers will catch
    // the techinal (though safe) OOB access.
    uint8_t vram[2][0x1000]; // 8kb
#endif
    uint8_t oam[0xA0]; // sprite mem

    // these are set when a hdma occurs (not a DMA or GDMA)
    uint16_t hdma_src_addr;
    uint16_t hdma_dst_addr;
    uint16_t hdma_length;

    // this is the internal line counter which is used as the index
    // for the window instead of IO_LY.
    uint8_t window_line;
    bool stat_line;

    union
    {
        #if GBC_ENABLE
        struct
        {
            uint32_t bg_colours[8][4]; // calculate the colours from the palette once.
            uint32_t obj_colours[8][4];

            uint8_t bg_palette[64]; // background palette memory.
            uint8_t obj_palette[64]; // sprite palette memory.
        } gbc;
        #endif

        struct
        {
            uint32_t bg_colours[20][4];
            uint32_t obj_colours[2][20][4];

            struct GB_PalCache bg_cache[20];
            struct GB_PalCache obj_cache[2][20];
        } dmg;
    } system;

    bool dirty_bg[8]; // only update the colours if the palette changes values.
    bool dirty_obj[8];
};

// todo: fix bad name
struct GB_MemMapEntry
{
    const uint8_t* ptr;
    uint16_t mask;
};

struct MBC_RomBankInfo
{
    struct GB_MemMapEntry entries[4];
};

struct MBC_RamBankInfo
{
    struct GB_MemMapEntry entries[2];
};

struct GB_Cart
{
    uint32_t rom_size; // set by the header
    uint32_t ram_size; // set by the header

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

    enum GB_MbcType type;
    uint8_t flags;
};

struct GB_Timer
{
    int16_t next_cycles;
};

struct GB_ApuCh1
{
    int16_t timer;
    uint16_t freq_shadow_register;

    uint8_t sweep_period;
    uint8_t sweep_shift;
    uint8_t duty;
    uint8_t starting_vol;
    uint8_t period;
    uint8_t freq_lsb;
    uint8_t freq_msb;
    uint8_t sweep_enabled;
    uint8_t duty_index;
    uint8_t volume;
    uint8_t length_counter;
    int8_t volume_timer;
    int8_t sweep_timer;

    bool sweep_negate;
    bool env_add_mode;
    bool length_enable;
    bool did_sweep_negate;
    bool disable_env;
};

struct GB_ApuCh2
{
    int16_t timer;

    uint8_t duty;
    uint8_t starting_vol;
    uint8_t period;
    uint8_t freq_lsb;
    uint8_t freq_msb;
    uint8_t duty_index;
    uint8_t volume;
    uint8_t length_counter;
    int8_t volume_timer;

    bool env_add_mode;
    bool length_enable;
    bool disable_env;
};

struct GB_ApuCh3
{
    uint16_t length_counter;
    int16_t timer;
    uint8_t vol_code;
    uint8_t freq_lsb;
    uint8_t freq_msb;
    uint8_t sample_buffer;
    uint8_t position_counter;

    bool dac_power;
    bool length_enable;
};

struct GB_ApuCh4
{
    int32_t timer;
    uint16_t lfsr;

    uint8_t starting_vol;
    uint8_t period;
    uint8_t clock_shift;
    uint8_t divisor_code;
    uint8_t volume;
    uint8_t length_counter;
    int8_t volume_timer;

    bool env_add_mode;
    bool width_mode;
    bool length_enable;
    bool disable_env;
};

struct GB_ApuCallbackData
{
    uint8_t ch1[2];
    uint8_t ch2[2];
    uint8_t ch3[2];
    uint8_t ch4[2];
};

struct GB_Apu
{
    uint16_t next_sample_cycles;

    struct GB_ApuCh1 ch1; // square 1
    struct GB_ApuCh2 ch2; // square 2
    struct GB_ApuCh3 ch3; // wave
    struct GB_ApuCh4 ch4; // noise

    uint8_t frame_sequencer_counter;
};

struct GB_mem
{
    uint8_t io[0x80]; // easier to have io as an array than individual bytes
    uint8_t hram[0x80]; // 0x7F + IE reg
#if GBC_ENABLE
    uint8_t wram[8][0x1000]; // extra 6 banks in gbc
#else
    uint8_t wram[2][0x1000]; // 2 banks of 4KiB
#endif

    uint8_t vbk;
    uint8_t svbk;
};

// TODO: this struct needs to be re-organised.
// atm, i've just been dumping vars in here as one big container,
// which works fine, though, it's starting to get messy, and could be
// *slightly* optimised by putting common accessed vars next to each other.
struct GB_Core
{
    struct GB_MemMapEntry mmap[0x10];

    int64_t cycles_left_to_run;

    struct GB_mem mem;
    struct GB_Cpu cpu;
    struct GB_Ppu ppu;
    struct GB_Apu apu;
    struct GB_Cart cart;
    struct GB_Timer timer;
    struct GB_Joypad joypad;

    struct GB_PaletteEntry palette; /* default */

    enum GB_SystemType system_type; // set by the rom itself

    struct GB_Config config;

    const uint8_t* rom;
    size_t rom_size; // set by the user

    uint8_t* ram;
    size_t ram_size; // set by the user

    void* pixels;
    uint32_t stride;
    uint8_t bpp;

    // todo: does this need it's own user data?
    GB_serial_transfer_t link_cable;
    void* link_cable_user_data;
    bool is_master;

    struct GB_UserCallbacks callback;
};

// i decided that the ram usage / statefile size is less important
// than code complexity, reliablility and speed.
// because of this, even if the game has no ram (tetris), it will
// still take up 128k.
// however *most* games do have ram, and fixing the size removes the
// need to allocate and simplifies savestate / rewinding *a lot*.
// further, compressed empty arrays often take up no space at all
struct GB_State
{
    uint16_t magic;
    uint16_t version;
    uint32_t size;
    uint8_t gbc_enabled;
    uint8_t sgb_enabled;
    uint8_t reserved[0xA];

    struct GB_mem mem;
    struct GB_Cpu cpu;
    struct GB_Ppu ppu;
    struct GB_Apu apu;
    struct GB_Cart cart;
    struct GB_Timer timer;

    uint8_t sram[GB_SAVE_SIZE_MAX];
};

struct GB_CartName
{
    char name[0x11]; // this is NULL terminated
};

#ifdef __cplusplus
}
#endif

#endif // GB_TYPES_H
