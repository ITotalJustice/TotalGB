#include "libretro.h"
#include "types.h"
#include <gb.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


enum
{
    FPS = 60,
    SAMPLE_RATE = 48000,
    FRAMEBUFFER_W = GB_SCREEN_WIDTH,
    FRAMEBUFFER_H = GB_SCREEN_HEIGHT,
};


static struct GB_Core gb = {0};

// double buffered
static uint16_t* framebuffers[2] = {0};
static uint8_t framebuffer_index = 0;

static uint8_t rom_data[GB_ROM_SIZE_MAX] = {0};
static size_t rom_size = 0;
static uint8_t sram_data[GB_SAVE_SIZE_MAX] = {0};
static size_t sram_size = 0;
static bool has_sram = false;

static retro_video_refresh_t video_cb = NULL;
static retro_audio_sample_t audio_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_environment_t environ_cb = NULL;
static retro_input_poll_t input_poll_cb = NULL;
static retro_input_state_t input_state_cb = NULL;


static uint32_t core_on_colour(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b)
{
    (void)user;

    uint16_t R = 0, G = 0, B = 0;

    switch (type)
    {
        // input is 888
        case GB_ColourCallbackType_DMG:
            R = r >> 3;
            G = g >> 2;
            B = b >> 3;
            break;

        // input is 555
        case GB_ColourCallbackType_GBC:
            R = r;
            G = g << 1;
            B = b;
            break;
    }

    return (R << 11) | (G << 5) | (B << 0);
}

static void core_on_vblank(void* user)
{
    (void)user;

    framebuffer_index ^= 1;
    GB_set_pixels(&gb, framebuffers[framebuffer_index], FRAMEBUFFER_W, 16);
}

static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    (void)user;

    const int16_t left = (data->ch1[0] + data->ch2[0] + data->ch3[0] + data->ch4[0]) * data->left_amp;
    const int16_t right = (data->ch1[1] + data->ch2[1] + data->ch3[1] + data->ch4[1]) * data->right_amp;

    // would x128, however we have to divide the final sample by 4 anyway beforehand
    // so instead of dive then mult, just mult
    audio_cb(left * 32, right * 32);
}

void retro_init(void)
{
    GB_init(&gb);
    GB_set_apu_callback(&gb, core_on_apu, NULL, SAMPLE_RATE);
    GB_set_vblank_callback(&gb, core_on_vblank, NULL);
    GB_set_colour_callback(&gb, core_on_colour, NULL);
}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(struct retro_system_info));
    info->library_name = "TotalGB";
    info->library_version = "0.0.1";
    info->valid_extensions = "gb|gbc|dmg|zip";
    info->need_fullpath = false;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    framebuffers[0] = calloc(FRAMEBUFFER_W * FRAMEBUFFER_H, sizeof(uint16_t));
    framebuffers[1] = calloc(FRAMEBUFFER_W * FRAMEBUFFER_H, sizeof(uint16_t));

    GB_set_pixels(&gb, framebuffers[framebuffer_index], FRAMEBUFFER_W, 16);

    info->timing.fps = FPS;
    info->timing.sample_rate = SAMPLE_RATE;

    info->geometry.aspect_ratio = (float)FRAMEBUFFER_W / (float)FRAMEBUFFER_H;
    info->geometry.base_width = FRAMEBUFFER_W;
    info->geometry.base_height = FRAMEBUFFER_H;
    info->geometry.max_width = FRAMEBUFFER_W;
    info->geometry.max_height = FRAMEBUFFER_H;

    enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_RGB565;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}

// savestate size
size_t retro_serialize_size(void)
{
    return sizeof(struct GB_State);
}

bool retro_serialize(void *data, size_t size)
{
    return GB_savestate(&gb, (struct GB_State*)data);
}

bool retro_unserialize(const void *data, size_t size)
{
    return GB_loadstate(&gb, (const struct GB_State*)data);
}

bool retro_load_game(const struct retro_game_info *game)
{
    // todo: support file loading
    if (game->size > sizeof(rom_data) || !game->size || !game->data)
    {
        return false;
    }

    struct GB_RomInfo info = {0};

    if (!GB_get_rom_info(game->data, game->size, &info))
    {
        return false;
    }

    if (info.ram_size > sizeof(sram_data))
    {
        return false;
    }

    enum { SRAM_MASK = MBC_FLAGS_BATTERY | MBC_FLAGS_BATTERY };

    sram_size = info.ram_size;
    has_sram = (info.flags & SRAM_MASK) == SRAM_MASK;

    // should probably allocate sram instead of using fixed buffer
    GB_set_sram(&gb, sram_data, sram_size);

    memcpy(rom_data, game->data, game->size);
    rom_size = game->size;

    return GB_loadrom(&gb, rom_data, rom_size);
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    // todo: support whatever this thing is
    return false;
}

void retro_unload_game(void)
{
    free(framebuffers[0]); framebuffers[0] = NULL;
    free(framebuffers[1]); framebuffers[1] = NULL;
}

// reset game
void retro_reset(void)
{
    GB_loadrom(&gb, rom_data, rom_size);
}

unsigned retro_get_region(void)
{
    return 0;
}

void *retro_get_memory_data(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return sram_data;

        case RETRO_MEMORY_SYSTEM_RAM:
            return gb.mem.wram;

        case RETRO_MEMORY_VIDEO_RAM:
            return gb.ppu.vram;
    }

    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return has_sram ? sram_size : 0;

        case RETRO_MEMORY_SYSTEM_RAM:
            return sizeof(gb.mem.wram);

        case RETRO_MEMORY_VIDEO_RAM:
            return sizeof(gb.ppu.vram);
    }

    return 0;
}

enum
{
    RA_JOYPAD_B      = 1 << 0,
    RA_JOYPAD_Y      = 1 << 1,
    RA_JOYPAD_SELECT = 1 << 2,
    RA_JOYPAD_START  = 1 << 3,
    RA_JOYPAD_UP     = 1 << 4,
    RA_JOYPAD_DOWN   = 1 << 5,
    RA_JOYPAD_LEFT   = 1 << 6,
    RA_JOYPAD_RIGHT  = 1 << 7,
    RA_JOYPAD_A      = 1 << 8,
    RA_JOYPAD_X      = 1 << 9,
    RA_JOYPAD_L      = 1 << 10,
    RA_JOYPAD_R      = 1 << 11,
    RA_JOYPAD_L2     = 1 << 12,
    RA_JOYPAD_R2     = 1 << 13,
    RA_JOYPAD_L3     = 1 << 14,
    RA_JOYPAD_R3     = 1 << 15,
};

void retro_run(void)
{
    // handle input
    input_poll_cb();

    uint16_t buttons = 0;

    // loop through all buttons on the controller
    for (uint8_t num_buttons = 0; num_buttons < 15; ++num_buttons)
    {
        // check which buttons are down
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, num_buttons))
        {
            // see above enum
            buttons |= 1 << num_buttons;
        }
    }

    GB_set_buttons(&gb, GB_BUTTON_A, buttons & RA_JOYPAD_A);
    GB_set_buttons(&gb, GB_BUTTON_B, buttons & RA_JOYPAD_B);
    GB_set_buttons(&gb, GB_BUTTON_UP, buttons & RA_JOYPAD_UP);
    GB_set_buttons(&gb, GB_BUTTON_DOWN, buttons & RA_JOYPAD_DOWN);
    GB_set_buttons(&gb, GB_BUTTON_LEFT, buttons & RA_JOYPAD_LEFT);
    GB_set_buttons(&gb, GB_BUTTON_RIGHT, buttons & RA_JOYPAD_RIGHT);
    GB_set_buttons(&gb, GB_BUTTON_START, buttons & RA_JOYPAD_START);
    GB_set_buttons(&gb, GB_BUTTON_SELECT, buttons & RA_JOYPAD_SELECT);

    // run for a frame
    GB_run(&gb, GB_FRAME_CPU_CYCLES);

    // render
    video_cb(framebuffers[framebuffer_index ^ 1], FRAMEBUFFER_W, FRAMEBUFFER_H, sizeof(uint16_t) * FRAMEBUFFER_W);
}
