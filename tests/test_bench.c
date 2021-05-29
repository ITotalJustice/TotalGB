#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <gb.h>


static struct GB_Core gameboy;
static uint16_t core_pixels[144][160];
static uint8_t rom_data[1024 * 1024 * 4];
static size_t rom_size = 0;


static void core_on_vblank(struct GB_Core* gb, void* user)
{
    (void)gb; (void)user;
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

    if (!GB_loadrom(&gameboy, rom_data, rom_size))
    {
        return -1;
    }

    GB_set_vblank_callback(&gameboy, core_on_vblank, NULL);
    GB_set_pixels(&gameboy, core_pixels, GB_SCREEN_WIDTH);

    for (;;)
    {
        GB_run_frame(&gameboy);
    }

    return 0;
}
