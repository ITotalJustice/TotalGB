#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <ogcsys.h>
#include <fat.h>
#include <gb.h>

// this is for testing. will add a file loader soon
#include "../../roms/rom.c"


// SOURCE-AUDIO: https://libogc.devkitpro.org/audio_8h.html
// audio code is based on MGBA wii port (thanks!)
// framebuffer code is based on the FB example from devkitpro gc examples!


#define BUFFERS (6)
#define SAMPLES (3840 * 2)

// docs recomend that the data is double buffered!
struct AudioBuffer
{
    uint16_t samples[SAMPLES] __attribute__((__aligned__(32)));;
    volatile int size;
};

static volatile int audio_buffer_index = 0;
static volatile int audio_buffer_next_index = 0;
static struct AudioBuffer audio_buffers[BUFFERS] = {0};

static struct GB_Core gameboy;
static uint32_t* pixels = NULL;

/*** 2D Video Globals ***/
static GXRModeObj* vmode;        /*** Graphics Mode Object ***/
static u32* xfb[2] = { NULL, NULL };    /*** Framebuffers ***/
static int whichfb = 0;        /*** Frame buffer toggle ***/


// from flip.c example in dkp-gc examples!
static u32 CvtRGB(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
    int y1, cb1, cr1, y2, cb2, cr2, cb, cr;

    y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
    cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
    cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;

    y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
    cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
    cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;

    cb = (cb1 + cb2) >> 1;
    cr = (cr1 + cr2) >> 1;

    return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

static uint32_t core_on_colour(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b)
{
    return CvtRGB(
        r << 3, g << 3, b << 3,
        r << 3, g << 3, b << 3
    );
}

static void core_on_vblank(void* user)
{
    // center the image
    const int cx = (640 - 160) / 2;
    const int cy = (480 - 144) / 2;

    for (int y = 0; y < 144; ++y)
    {
        int yoff = (640 * y) + cy;

        // copy 1 line at a time
        memcpy(xfb[whichfb] + yoff + cx, pixels + (160 * y), 160 * 4);
    }
}

static void run()
{
    GB_run_frame(&gameboy);
}

static void events()
{
    PAD_ScanPads();
    const u32 buttons = PAD_ButtonsHeld(0);

    // this is a lazy way to set buttons.
    GB_set_buttons(&gameboy, GB_BUTTON_A, buttons & PAD_BUTTON_A);
    GB_set_buttons(&gameboy, GB_BUTTON_B, buttons & PAD_BUTTON_B);
    GB_set_buttons(&gameboy, GB_BUTTON_START, buttons & PAD_BUTTON_START);
    // GB_set_buttons(&gameboy, GB_BUTTON_SELECT, buttons & PAD_BUTTON_Z);
    GB_set_buttons(&gameboy, GB_BUTTON_UP, buttons & PAD_BUTTON_UP);
    GB_set_buttons(&gameboy, GB_BUTTON_DOWN, buttons & PAD_BUTTON_DOWN);
    GB_set_buttons(&gameboy, GB_BUTTON_LEFT, buttons & PAD_BUTTON_LEFT);
    GB_set_buttons(&gameboy, GB_BUTTON_RIGHT, buttons & PAD_BUTTON_RIGHT);
}

static void render()
{
    whichfb ^= 1;

    VIDEO_SetNextFramebuffer(xfb[whichfb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

static void on_audio_dma()
{
    struct AudioBuffer* buffer = &audio_buffers[audio_buffer_index];

    if (buffer->size != SAMPLES)
    {
        return;
    }

    // we need to flush because we don't want it to use cached data
    DCFlushRange(buffer->samples, sizeof(buffer->samples));
    // this starts an audio dma transfer (addr, len)
    AUDIO_InitDMA((u32) buffer->samples, sizeof(buffer->samples));

    buffer->size = 0;
    audio_buffer_index = (audio_buffer_index + 1) % BUFFERS;
}

static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    struct AudioBuffer* buffer = &audio_buffers[audio_buffer_next_index];
    
    if (buffer->size == SAMPLES)
    {
        return;
    }

    uint16_t left = ((uint16_t)data->ch1[0] + (uint16_t)data->ch2[0] + (uint16_t)data->ch3[0] + (uint16_t)data->ch4[0]);
    uint16_t right = ((uint16_t)data->ch1[1] + (uint16_t)data->ch2[1] + (uint16_t)data->ch3[1] + (uint16_t)data->ch4[1]);

    if (left) { left <<= 4; }
    if (right) { right <<= 4; }

    buffer->samples[buffer->size++] = right;
    buffer->samples[buffer->size++] = left;

    if (buffer->size == SAMPLES)
    {
        audio_buffer_next_index = (audio_buffer_next_index + 1) % BUFFERS;

        if (!AUDIO_GetDMAEnableFlag())
        {
            on_audio_dma();
            AUDIO_StartDMA();
        }
    }
}

int main(int argc, char** argv)
{
    pixels = (uint32_t*)malloc(160*144*4);

    VIDEO_Init();

    PAD_Init();

    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    AUDIO_RegisterDMACallback(on_audio_dma);

    vmode = VIDEO_GetPreferredMode(NULL);

    VIDEO_Configure(vmode);

    xfb[0] = (u32*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
    xfb[1] = (u32*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

    VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);

    VIDEO_SetNextFramebuffer(xfb[0]);

    VIDEO_SetBlack (0);

    VIDEO_Flush();
    VIDEO_WaitVSync();

    // examples show if interlaced, wait for vblank again...
    if (vmode->viTVMode & VI_NON_INTERLACE)
    {
        VIDEO_WaitVSync();
    }

    GB_init(&gameboy);
    GB_set_apu_callback(&gameboy, core_on_apu, NULL, 48000);
    GB_set_vblank_callback(&gameboy, core_on_vblank, NULL);
    GB_set_colour_callback(&gameboy, core_on_colour, NULL);
    GB_set_pixels(&gameboy, pixels, GB_SCREEN_WIDTH, 32);
    GB_loadrom(&gameboy, rom, sizeof(rom));

    for (;;)
    {
        events();
        run();
        render();
    }

    return 0;
}
