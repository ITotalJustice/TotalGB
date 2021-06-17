#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <gb.h>


#define GB_AUDIO_FREQ (48000)


static struct GB_Core gameboy;
static uint16_t core_pixels[144][160];
static uint8_t rom_data[1024 * 1024 * 4];
static uint8_t sram_data[1024 * 1024 * 4];
static size_t rom_size = 0;


static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    (void)user; (void)data;
}

static void core_on_vblank(void* user)
{
    (void)user;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return -1;
    }

    printf("loading rom %s\n", argv[1]);
    
    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        return -1;
    }

    rom_size = fread(rom_data, 1, sizeof(rom_data), f);
    printf("read %lu\n", rom_size);
    fclose(f);

    if (!GB_init(&gameboy))
    {
        return -1;
    }

    GB_set_apu_callback(&gameboy, core_on_apu, NULL, GB_AUDIO_FREQ);
    GB_set_vblank_callback(&gameboy, core_on_vblank, NULL);
    GB_set_pixels(&gameboy, core_pixels, GB_SCREEN_WIDTH, 16);

    GB_set_sram(&gameboy, sram_data, sizeof(sram_data));

    if (!GB_loadrom(&gameboy, rom_data, rom_size))
    {
        return -1;
    }

    size_t fps = 0;
    clock_t start_t, end_t;
    start_t = clock();

    for (;;)
    {
        GB_run_frame(&gameboy);
        ++fps;
    
        end_t = clock();

        if ((end_t - start_t) / CLOCKS_PER_SEC >= 1)
        {
            start_t = end_t;
            printf("[FPS] %zu\n", fps);
            fps = 0;
        }
    }

    return 0;
}
