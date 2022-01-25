// TODO: cleanup this file, its a mess.
// overtime i've just dumped functions here that i wanted to add,
// with the idea that i'd eventually cleanup and organise this file.

#include "gb.h"
#include "internal.h"
#include "apu/apu.h"
#include "tables/palette_table.h"
#include "types.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>


#define ROM_SIZE_MULT 0x8000


bool GB_init(struct GB_Core* gb)
{
    if (!gb)
    {
        return false;
    }

    memset(gb, 0, sizeof(struct GB_Core));

    return true;
}

void GB_quit(struct GB_Core* gb)
{
    assert(gb);
    UNUSED(gb);
}

void GB_reset(struct GB_Core* gb)
{
    memset(&gb->mem, 0, sizeof(gb->mem));
    memset(&gb->cpu, 0, sizeof(gb->cpu));
    memset(&gb->apu, 0, sizeof(gb->apu));
    memset(&gb->ppu, 0, sizeof(gb->ppu));
    memset(&gb->timer, 0, sizeof(gb->timer));
    memset(&gb->joypad, 0, sizeof(gb->joypad));
    memset(IO, 0xFF, sizeof(IO));

    GB_update_all_colours_gb(gb);

    gb->joypad.var = 0xFF;
    gb->ppu.next_cycles = 0;
    gb->timer.next_cycles = 0;
    gb->cpu.cycles = 0;
    gb->cpu.halt = 0;
    gb->cpu.ime = 0;

    gb->mem.vbk = 0;
    gb->mem.svbk = 1;

    // CPU
    GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_SP, 0xFFFE);
    GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_PC, 0x0100);

    // TODO: have the user set which system it wants to emulate (dmg / gbc)
    // the effects some registers below, for example, a dmg game running on a
    // gbc will be able to access some of the gbc registers.

    // IO
    IO_TIMA = 0x00;
    IO_TMA = 0x00;
    IO_TAC = 0x00;
    IO_LCDC = 0x91;
    IO_STAT = 0x00;
    IO_SCY = 0x00;
    IO_SCX = 0x00;
    IO_LY = 0x00;
    IO_LYC = 0x00;
    IO_BGP = 0xFC;
    IO_WY = 0x00;
    IO_WX = 0x00;
    IO_IF = 0x00;
    IO_IE = 0x00;
    IO_SC = 0x00;
    IO_SB = 0x00;
    IO_DIV_LOWER = 0x00;
    IO_DIV_UPPER = 0x00;

    IO[0x10] = 0x80;//   ; NR10
    IO[0x11] = 0xBF;//   ; NR11
    IO[0x12] = 0xF3;//   ; NR12
    IO[0x14] = 0xBF;//   ; NR14
    IO[0x16] = 0x3F;//   ; NR21
    IO[0x17] = 0x00;//   ; NR22
    IO[0x19] = 0xBF;//   ; NR24
    IO[0x1A] = 0x7F;//   ; NR30
    IO[0x1B] = 0xFF;//   ; NR31
    IO[0x1C] = 0x9F;//   ; NR32
    IO[0x1E] = 0xBF;//   ; NR33
    IO[0x20] = 0xFF;//   ; NR41
    IO[0x21] = 0x00;//   ; NR42
    IO[0x22] = 0x00;//   ; NR43
    IO[0x23] = 0xBF;//   ; NR44
    IO[0x24] = 0x77;//   ; NR50
    IO[0x25] = 0xF3;//   ; NR51
    IO[0x26] = 0xF1;//   ; NR52

    // triggering the channels causes a high pitch sound effect to be played
    // at start of most games so disabled for now.
    // TODO: run the bios and check the state of the core after 0x50 write
    // and set the internal values to match that!
    #if 1
    on_nr10_write(gb, 0x80);
    on_nr11_write(gb, 0xBF);
    on_nr12_write(gb, 0xF3);
    // on_nr14_write(gb, 0xBF);
    on_nr21_write(gb, 0x3F);
    on_nr22_write(gb, 0x00);
    // on_nr24_write(gb, 0xBF);
    on_nr30_write(gb, 0x7F);
    on_nr31_write(gb, 0xFF);
    on_nr32_write(gb, 0x9F);
    on_nr33_write(gb, 0xBF);
    on_nr41_write(gb, 0xFF);
    on_nr42_write(gb, 0x00);
    on_nr43_write(gb, 0x00);
    // on_nr44_write(gb, 0xBF);
    #endif

    const uint8_t dmg_wave_ram[16] =
    {
        0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C,
        0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA,
    };

    const uint8_t gbc_wave_ram[16] =
    {
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    };

    // cpu register initial values taken from TCAGBD.pdf
    // SOURCE: https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf

    switch (GB_get_system_type(gb))
    {
        case GB_SYSTEM_TYPE_DMG:
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_AF, 0x01B0);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_BC, 0x0013);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_DE, 0x00D8);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_HL, 0x014D);
            memcpy(IO_WAVE_TABLE, dmg_wave_ram, sizeof(dmg_wave_ram));
            break;

        case GB_SYSTEM_TYPE_SGB:
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_AF, 0x0100);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_BC, 0x0014);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_DE, 0x0000);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_HL, 0xC060);
            memcpy(IO_WAVE_TABLE, dmg_wave_ram, sizeof(dmg_wave_ram));
            break;

        case GB_SYSTEM_TYPE_GBC:
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_AF, 0x1180);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_BC, 0x0000);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_DE, 0xFF56);
            GB_cpu_set_register_pair(gb, GB_CPU_REGISTER_PAIR_HL, 0x000D);
            IO_SVBK = 0x01;
            IO_VBK = 0x00;
            IO_BCPS = 0x00;
            IO_OCPS = 0x00;
            IO_OPRI = 0xFE;
            IO_KEY1 = 0x7E;
            IO_72 = 0x00;
            IO_73 = 0x00;
            IO_74 = 0x00;
            IO_75 = 0x8F;
            IO_76 = 0x00;
            IO_77 = 0x00;
            memcpy(IO_WAVE_TABLE, dmg_wave_ram, sizeof(gbc_wave_ram));
            break;
    }
}

static const char* cart_type_str(const uint8_t type)
{
    switch (type)
    {
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

static void cart_header_print(const struct GB_CartHeader* header)
{
    (void)cart_type_str;

    GB_log("\nROM HEADER INFO\n");

    struct GB_CartName cart_name;
    GB_get_rom_name_from_header(header, &cart_name);

    GB_log("\tTITLE: %s\n", cart_name.name);
    // GB_log("\tNEW LICENSEE CODE: 0x%02X\n", header->new_licensee_code);
    GB_log("\tSGB FLAG: 0x%02X\n", header->sgb_flag);
    GB_log("\tCART TYPE: %s\n", cart_type_str(header->cart_type));
    GB_log("\tCART TYPE VALUE: 0x%02X\n", header->cart_type);
    GB_log("\tROM SIZE: 0x%02X\n", header->rom_size);
    GB_log("\tRAM SIZE: 0x%02X\n", header->ram_size);
    GB_log("\tHEADER CHECKSUM: 0x%02X\n", header->header_checksum);
    // GB_log("\tGLOBAL CHECKSUM: 0x%04X\n", header->global_checksum);

    uint8_t hash = 0, forth = 0;
    GB_get_rom_palette_hash_from_header(header, &hash, &forth);
    GB_log("\tHASH: 0x%02X, 0x%02X\n", hash, forth);
    GB_log("\n");
}

bool GB_get_rom_header_from_data(const uint8_t* data, struct GB_CartHeader* header)
{
    memcpy(header, data + GB_BOOTROM_SIZE, sizeof(struct GB_CartHeader));
    return true;
}

bool GB_get_rom_header(const struct GB_Core* gb, struct GB_CartHeader* header)
{
    if (!gb->rom || (gb->cart.ram_size < (sizeof(struct GB_CartHeader) + GB_BOOTROM_SIZE)))
    {
        return false;
    }

    return GB_get_rom_header_from_data(gb->rom, header);
}

static const struct GB_CartHeader* GB_get_rom_header_ptr_from_data(const uint8_t* data)
{
    return (const struct GB_CartHeader*)&data[GB_BOOTROM_SIZE];
}

const struct GB_CartHeader* GB_get_rom_header_ptr(const struct GB_Core* gb)
{
    return GB_get_rom_header_ptr_from_data(gb->rom);
}

bool GB_get_rom_palette_hash_from_header(const struct GB_CartHeader* header, uint8_t* hash, uint8_t* forth)
{
    if (!header || !hash || !forth)
    {
        return false;
    }

    uint8_t temp_hash = 0;
    for (uint16_t i = 0; i < sizeof(header->title); ++i)
    {
        temp_hash += header->title[i];
    }

    *hash = temp_hash;
    *forth = (uint8_t)header->title[0x3];

    return true;
}

bool GB_get_rom_palette_hash(const struct GB_Core* gb, uint8_t* hash, uint8_t* forth)
{
    if (!hash)
    {
        return false;
    }

    return GB_get_rom_palette_hash_from_header(
        GB_get_rom_header_ptr(gb),
        hash, forth
    );
}

bool GB_set_palette_from_palette(struct GB_Core* gb, const struct GB_PaletteEntry* palette)
{
    if (!palette)
    {
        return false;
    }

    gb->palette = *palette;

    return true;
}

void GB_set_render_palette_layer_config(struct GB_Core* gb, enum GB_RenderLayerConfig layer)
{
    gb->config.render_layer_config = layer;
}

void GB_set_rtc_update_config(struct GB_Core* gb, const enum GB_RtcUpdateConfig config)
{
    gb->config.rtc_update_config = config;
}

bool GB_set_rtc(struct GB_Core* gb, const struct GB_Rtc rtc)
{
    if (GB_has_mbc_flags(gb, MBC_FLAGS_RTC) == false)
    {
        return false;
    }

    gb->cart.rtc.S = rtc.S > 59 ? 59 : rtc.S;
    gb->cart.rtc.M = rtc.M > 59 ? 59 : rtc.M;
    gb->cart.rtc.H = rtc.H > 23 ? 23 : rtc.H;
    gb->cart.rtc.DL = rtc.DL;
    gb->cart.rtc.DH = rtc.DH & 0xC1; // only bit 0,6,7

    return true;
}

bool GB_has_mbc_flags(const struct GB_Core* gb, const uint8_t flags)
{
    return (gb->cart.flags & flags) == flags;
}

enum GB_SystemType GB_get_system_type(const struct GB_Core* gb)
{
    return gb->system_type;
}

bool GB_is_system_gbc(const struct GB_Core* gb)
{
    return GB_get_system_type(gb) == GB_SYSTEM_TYPE_GBC;
}

static const char* GB_get_system_type_string(const enum GB_SystemType type)
{
    switch (type)
    {
        case GB_SYSTEM_TYPE_DMG: return "GB_SYSTEM_TYPE_DMG";
        case GB_SYSTEM_TYPE_SGB: return "GB_SYSTEM_TYPE_SBC";
        case GB_SYSTEM_TYPE_GBC: return "GB_SYSTEM_TYPE_GBC";
    }

    return "NULL";
}

static void GB_set_system_type(struct GB_Core* gb, const enum GB_SystemType type)
{
    (void)GB_get_system_type_string;

    GB_log("[INFO] setting system type to %s\n", GB_get_system_type_string(type));
    gb->system_type = type;
}

static void on_set_builtin_palette(struct GB_Core* gb, struct PaletteEntry* p)
{
    if (!gb->callback.colour)
    {
        return;
    }

    for (uint8_t i = 0; i < 4; ++i)
    {
        gb->palette.BG[i] = gb->callback.colour(gb->callback.user_colour, GB_ColourCallbackType_DMG, p->BG[i].r, p->BG[i].g, p->BG[i].b);
    }

    for (uint8_t i = 0; i < 4; ++i)
    {
        gb->palette.OBJ0[i] = gb->callback.colour(gb->callback.user_colour, GB_ColourCallbackType_DMG, p->OBJ0[i].r, p->OBJ0[i].g, p->OBJ0[i].b);
    }

    for (uint8_t i = 0; i < 4; ++i)
    {
        gb->palette.OBJ1[i] = gb->callback.colour(gb->callback.user_colour, GB_ColourCallbackType_DMG, p->OBJ1[i].r, p->OBJ1[i].g, p->OBJ1[i].b);
    }
}

static void GB_setup_palette(struct GB_Core* gb, const struct GB_CartHeader* header)
{
    // this should only ever be called in NONE GBC system.
    assert(GB_is_system_gbc(gb) == false);

    struct PaletteEntry builtin_palette = {0};

    if (gb->config.palette_config == GB_PALETTE_CONFIG_USE_CUSTOM)
    {
        GB_set_palette_from_palette(gb, &gb->config.custom_palette);
    }
    else if (gb->config.palette_config == GB_PALETTE_CONFIG_NONE || (gb->config.palette_config & GB_PALETTE_CONFIG_USE_BUILTIN) == GB_PALETTE_CONFIG_USE_BUILTIN)
    {
        // attempt to fill set the palatte from the builtins...
        uint8_t hash = 0, forth = 0;
        // this will never fail...
        GB_get_rom_palette_hash_from_header(header, &hash, &forth);

        if (!palette_fill_from_hash(hash, forth, &builtin_palette))
        {
            // try and fallback to custom palette if the user has set it
            if ((gb->config.palette_config & GB_PALETTE_CONFIG_USE_CUSTOM) == GB_PALETTE_CONFIG_USE_CUSTOM)
            {
                GB_set_palette_from_palette(gb, &gb->config.custom_palette);
            }
            // otherwise use default palette...
            else
            {
                palette_fill_from_custom(CUSTOM_PALETTE_KGREEN, &builtin_palette);
                on_set_builtin_palette(gb, &builtin_palette);
            }
        }
        else
        {
            on_set_builtin_palette(gb, &builtin_palette);
        }
    }
}

void GB_set_pixels(struct GB_Core* gb, void* pixels, uint32_t stride, uint8_t bpp)
{
    gb->pixels = pixels;
    gb->stride = stride;
    gb->bpp = bpp;
}

void GB_set_sram(struct GB_Core* gb, uint8_t* ram, size_t size)
{
    gb->ram = ram;
    gb->ram_size = size;

    // if we have a rom loaded, re-map the ram banks.

    // TODO: this should be a func instead of checking the ptr here!
    if (gb->rom)
    {
        GB_update_ram_banks(gb);
    }
}

bool GB_get_rom_info(const uint8_t* data, size_t size, struct GB_RomInfo* info_out)
{
    // todo: should ensure the romsize is okay!
    (void)size;

    const struct GB_CartHeader* header = GB_get_rom_header_ptr_from_data(data);

    info_out->rom_size = ROM_SIZE_MULT << header->rom_size;

    if (!GB_get_cart_ram_size(header, &info_out->ram_size))
    {
        return false;
    }

    if (!GB_get_mbc_flags(header->cart_type, &info_out->flags))
    {
        return false;
    }

    return true;
}

bool GB_loadrom(struct GB_Core* gb, const uint8_t* data, size_t size)
{
    if (!data || !size)
    {
        return false;
    }

    if (size < GB_BOOTROM_SIZE + sizeof(struct GB_CartHeader))
    {
        return false;
    }

    if (size > UINT32_MAX)
    {
        return false;
    }

    const struct GB_CartHeader* header = GB_get_rom_header_ptr_from_data(data);

    cart_header_print(header);

    gb->cart.rom_size = ROM_SIZE_MULT << header->rom_size;

    if (gb->cart.rom_size > size)
    {
        return false;
    }

    enum
    {
        GBC_ONLY = 0xC0,
        GBC_AND_DMG = 0x80,
        // not much is known about these types
        // these are not checked atm, but soon will be...
        PGB_1 = 0x84,
        PGB_2 = 0x88,

        SGB_FLAG = 0x03,

        NEW_LICENSEE_USED = 0x33,
    };

    // todo: clean up the below spaghetti code
    const char gbc_flag = header->title[sizeof(header->title) - 1];

    if ((gbc_flag & GBC_ONLY) == GBC_ONLY)
    {
        // this can be either set to GBC or DMG mode, check if the
        // user has set a preffrence
        switch (gb->config.system_type_config)
        {
            case GB_SYSTEM_TYPE_CONFIG_NONE:
            case GB_SYSTEM_TYPE_CONFIG_GBC:
                #if GBC_ENABLE
                    GB_set_system_type(gb, GB_SYSTEM_TYPE_GBC);
                #else
                    GB_log("[ERROR] game is gbc only but emu is built without gbc!\n");
                    return false;
                #endif
                break;

            case GB_SYSTEM_TYPE_CONFIG_SGB:
                GB_log("GBC only game but set to SGB system via config...\n");
                GB_set_system_type(gb, GB_SYSTEM_TYPE_SGB);
                break;

            case GB_SYSTEM_TYPE_CONFIG_DMG:
                GB_log("[ERROR] only GBC system supported but config forces DMG system!\n");
                return false;
        }
    }
    else if ((gbc_flag & GBC_AND_DMG) == GBC_AND_DMG)
    {
        // this can be either set to GBC or DMG mode, check if the
        // user has set a preffrence
        switch (gb->config.system_type_config)
        {
            case GB_SYSTEM_TYPE_CONFIG_NONE:
            case GB_SYSTEM_TYPE_CONFIG_GBC:
                #if GBC_ENABLE
                    GB_set_system_type(gb, GB_SYSTEM_TYPE_GBC);
                #else
                    GB_log("rom supports GBC mode, however falling back to DMG mode...\n");
                    GB_set_system_type(gb, GB_SYSTEM_TYPE_DMG);
                #endif
                break;

            case GB_SYSTEM_TYPE_CONFIG_SGB:
                GB_log("rom supports GBC mode, however falling back to SGB mode...\n");
                GB_set_system_type(gb, GB_SYSTEM_TYPE_SGB);
                break;

            case GB_SYSTEM_TYPE_CONFIG_DMG:
                GB_set_system_type(gb, GB_SYSTEM_TYPE_DMG);
                break;
        }
    }
    else
    {
        switch (gb->config.system_type_config)
        {
            case GB_SYSTEM_TYPE_CONFIG_GBC:
                GB_log("[ERROR] only DMG system supported but config forces GBC system!\n");
                return false;

            // if the user wants SGB system, we can set it here as all gb
            // games work on the SGB.
            case GB_SYSTEM_TYPE_CONFIG_SGB:
                #if SGB_ENABLE
                    GB_set_system_type(gb, GB_SYSTEM_TYPE_SGB);
                #else
                    GB_log("rom supports SGB mode, however falling back to DMG mode...\n");
                    GB_set_system_type(gb, GB_SYSTEM_TYPE_DMG);
                #endif
                break;

            case GB_SYSTEM_TYPE_CONFIG_NONE:
            case GB_SYSTEM_TYPE_CONFIG_DMG:
                GB_set_system_type(gb, GB_SYSTEM_TYPE_DMG);
                break;
        }
    }

    // need to change the above if-else tree to support this
    // for now, try and detect SGB
    if (header->sgb_flag == SGB_FLAG && header->old_licensee_code == NEW_LICENSEE_USED)
    {
        GB_log("[INFO] game supports SGB!\n");
        #if SGB_ENABLE
            GB_set_system_type(gb, GB_SYSTEM_TYPE_SGB);
        #endif
    }

    // try and setup the mbc, this also implicitly sets up
    // gbc mode
    if (!GB_setup_mbc(gb, header))
    {
        GB_log("failed to setup mbc!\n");
        return false;
    }

    // todo: should add more checks before we get to this point!
    gb->rom = data;
    gb->rom_size = size;

    GB_reset(gb);
    GB_setup_mmap(gb);

    // set the palette!
    if (GB_is_system_gbc(gb) == false)
    {
        GB_setup_palette(gb, header);
    }

    return true;
}

bool GB_has_save(const struct GB_Core* gb)
{
    return (gb->cart.flags & (MBC_FLAGS_RAM | MBC_FLAGS_BATTERY)) == (MBC_FLAGS_RAM | MBC_FLAGS_BATTERY);
}

bool GB_has_rtc(const struct GB_Core* gb)
{
    return (gb->cart.flags & MBC_FLAGS_RTC) == MBC_FLAGS_RTC;
}

size_t GB_calculate_savedata_size(const struct GB_Core* gb)
{
    return gb->cart.ram_size;
}


enum { STATE_MAGIC = 0x6BCE };
enum { STATE_VER = 1 };

bool GB_savestate(const struct GB_Core* gb, struct GB_State* state)
{
    if (!state || !gb->rom)
    {
        return false;
    }

    state->magic = STATE_MAGIC;
    state->version = STATE_VER;
    state->size = sizeof(struct GB_State);

    // the structs will have different sizes based on if built with sgb / gbc
    state->gbc_enabled = GBC_ENABLE;
    state->sgb_enabled = SGB_ENABLE;

    memset(state->reserved, 0xFF, sizeof(state->reserved));

    memcpy(&state->mem, &gb->mem, sizeof(state->mem));
    memcpy(&state->cpu, &gb->cpu, sizeof(state->cpu));
    memcpy(&state->ppu, &gb->ppu, sizeof(state->ppu));
    memcpy(&state->apu, &gb->apu, sizeof(state->apu));
    memcpy(&state->cart, &gb->cart, sizeof(state->cart));
    memcpy(&state->timer, &gb->timer, sizeof(state->timer));

    // set array to zero to allow for unused space to be better compressed
    memset(state->sram, 0, sizeof(state->sram));

    const size_t sram_size = GB_calculate_savedata_size(gb);

    if (sram_size && sram_size <= gb->ram_size && gb->ram)
    {
        memcpy(state->sram, gb->ram, sram_size);
    }

    return true;
}

bool GB_loadstate(struct GB_Core* gb, const struct GB_State* state)
{
    if (!state || !gb->rom)
    {
        return false;
    }

    if (state->magic != STATE_MAGIC || state->version != STATE_VER || state->size != sizeof(struct GB_State))
    {
        return false;
    }

    if (state->gbc_enabled != GBC_ENABLE || state->sgb_enabled != SGB_ENABLE)
    {
        return false;
    }

    memcpy(&gb->mem, &state->mem, sizeof(gb->mem));
    memcpy(&gb->cpu, &state->cpu, sizeof(gb->cpu));
    memcpy(&gb->ppu, &state->ppu, sizeof(gb->ppu));
    memcpy(&gb->apu, &state->apu, sizeof(gb->apu));
    memcpy(&gb->cart, &state->cart, sizeof(gb->cart));
    memcpy(&gb->timer, &state->timer, sizeof(gb->timer));

    const size_t sram_size = GB_calculate_savedata_size(gb);

    if (sram_size && sram_size <= gb->ram_size && gb->ram)
    {
        memcpy(gb->ram, state->sram, sram_size);
    }

    // we need to reload mmaps
    GB_setup_mmap(gb);
    // reload colours!
    GB_update_all_colours_gb(gb);

    return true;
}

void GB_enable_interrupt(struct GB_Core* gb, const enum GB_Interrupts interrupt)
{
    IO_IF |= interrupt;
}

void GB_disable_interrupt(struct GB_Core* gb, const enum GB_Interrupts interrupt)
{
    IO_IF &= ~(interrupt);
}

void GB_set_apu_freq(struct GB_Core* gb, unsigned freq)
{
    if (freq)
    {
        const float r = (float)GB_CPU_CYCLES / (float)freq;
        // this basically rounds up the decimal, so 43.5 will be 44
        gb->callback.apu_data.freq_reload = (uint16_t)(r + 0.5f);
    }
    else
    {
        gb->callback.apu_data.freq_reload = 0;
    }
}

void GB_set_apu_callback(struct GB_Core* gb, GB_apu_callback_t cb, void* user, unsigned freq)
{
    gb->callback.apu = cb;
    gb->callback.user_apu = user;

    GB_set_apu_freq(gb, freq);
}

void GB_set_vblank_callback(struct GB_Core* gb, GB_vblank_callback_t cb, void* user)
{
    gb->callback.vblank = cb;
    gb->callback.user_vblank = user;
}

void GB_set_hblank_callback(struct GB_Core* gb, GB_hblank_callback_t cb, void* user)
{
    gb->callback.hblank = cb;
    gb->callback.user_hblank = user;
}

void GB_set_dma_callback(struct GB_Core* gb, GB_dma_callback_t cb, void* user)
{
    gb->callback.dma = cb;
    gb->callback.user_dma = user;
}

void GB_set_halt_callback(struct GB_Core* gb, GB_halt_callback_t cb, void* user)
{
    gb->callback.halt = cb;
    gb->callback.user_halt = user;
}

void GB_set_stop_callback(struct GB_Core* gb, GB_stop_callback_t cb, void* user)
{
    gb->callback.stop = cb;
    gb->callback.user_stop = user;
}

void GB_set_colour_callback(struct GB_Core* gb, GB_colour_callback_t cb, void* user)
{
    gb->callback.colour = cb;
    gb->callback.user_colour = user;
}

void GB_set_rom_bank_callback(struct GB_Core* gb, GB_rom_bank_callback_t cb, void* user)
{
    gb->callback.rom_bank = cb;
    gb->callback.user_rom_bank = user;
}

#if GB_DEBUG
void GB_set_read_callback(struct GB_Core* gb, GB_read_callback_t cb, void* user)
{
    gb->callback.read = cb;
    gb->callback.user_read = user;
}

void GB_set_write_callback(struct GB_Core* gb, GB_write_callback_t cb, void* user)
{
    gb->callback.write = cb;
    gb->callback.user_write = user;
}
#endif // #if GB_DEBUG

uint8_t GB_read(struct GB_Core* gb, uint16_t addr)
{
    return GB_read8(gb, addr);
}

void GB_write(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    GB_write8(gb, addr, value);
}

uint8_t GB_get_ram_bank(struct GB_Core* gb)
{
    return gb->cart.ram_bank;
}

uint8_t GB_get_wram_bank(struct GB_Core* gb)
{
    return IO_SVBK;
}

void GB_set_ram_bank(struct GB_Core* gb, uint8_t bank)
{
    gb->cart.ram_bank = bank;

    GB_update_ram_banks(gb);
}

void GB_set_wram_bank(struct GB_Core* gb, uint8_t bank)
{
    IO_SVBK = bank & 0x7;
    gb->mem.svbk = bank & 0x7;

    GB_update_wram_banks(gb);
}

void GB_run(struct GB_Core* gb, uint32_t tcycles)
{
    assert(gb);

    // this is used so that the frontend can easily
    // modify the cycles via callback
    gb->cycles_left_to_run += tcycles;

    while (gb->cycles_left_to_run > 0)
    {
        const uint16_t cycles = GB_cpu_run(gb, 0 /*unused*/);

        GB_timer_run(gb, cycles);
        GB_ppu_run(gb, cycles >> gb->cpu.double_speed);
        GB_apu_run(gb, cycles >> gb->cpu.double_speed);

        assert(gb->cpu.double_speed == 1 || gb->cpu.double_speed == 0);

        gb->cycles_left_to_run -= cycles >> gb->cpu.double_speed;
    }
}
