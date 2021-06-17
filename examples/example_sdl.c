// this is a small-ish example of how you would use my GB_core
// and how to write a basic "frontend".

#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <gb.h>
#if GB_ZROM
    #include <zrom.h>
    static uint8_t zrom_mem_pool[ZROM_BANK_SIZE * 7];
#endif
#include <SDL.h>


enum
{
    WIDTH = 160,
    HEIGHT = 144,

    VOLUME = SDL_MIX_MAXVOLUME / 16,
    SAMPLES = 1024,
    SDL_AUDIO_FREQ = 48000,
    GB_AUDIO_FREQ = SDL_AUDIO_FREQ,

    AUDIO_SLEEP = 2,
};


static struct GB_Core gameboy;
static uint32_t core_pixels[144][160];
static uint8_t* rom_data = NULL;
static size_t rom_size = 0;
static bool running = true;
static int scale = 4;
static int speed = 1;
static int frameskip_counter = 0;

static int sram_fd = -1;
static uint8_t* sram_data = NULL;
static size_t sram_size = 0;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Texture* prev_texture = NULL;
static SDL_AudioDeviceID audio_device = 0;
static SDL_Rect rect = {0};
static SDL_PixelFormat* pixel_format = NULL;


static void run()
{
    for (int i = 0; i < speed; ++i)
    {
        // check if we should skip next frame
        if (i + 1 != speed)
        {
            GB_skip_next_frame(&gameboy);
        }

        GB_run_frame(&gameboy);
    }
}

static bool is_fullscreen()
{
    const int flags = SDL_GetWindowFlags(window);

    // check if we are already in fullscreen mode
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void setup_rect(int w, int h)
{
    if (!w || !h)
    {
        return;
    }
    
    const int scale_w = w / WIDTH;
    const int scale_h = h / HEIGHT;

    // get the min scale
    const int min_scale = scale_w < scale_h ? scale_w : scale_h;

    rect.w = WIDTH * min_scale;
    rect.h = HEIGHT * min_scale;
    rect.x = (w - rect.w);
    rect.y = (h - rect.h);

    // don't divide by zero!
    if (rect.x > 0) rect.x /= 2;
    if (rect.y > 0) rect.y /= 2;
}

static void scale_screen()
{
    SDL_SetWindowSize(window, WIDTH * scale, HEIGHT * scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void toggle_fullscreen()
{
    // check if we are already in fullscreen mode
    if (is_fullscreen())
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

static void on_ctrl_key_event(const SDL_KeyboardEvent* e, bool down)
{
    if (down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_EQUALS:
            case SDL_SCANCODE_KP_PLUS:
                ++scale;
                scale_screen();
                break;

            case SDL_SCANCODE_MINUS:
            case SDL_SCANCODE_KP_PLUSMINUS:
            case SDL_SCANCODE_KP_MINUS:
                scale = scale > 0 ? scale - 1 : 1;
                scale_screen();
                break;

            case SDL_SCANCODE_1:
            case SDL_SCANCODE_2:
            case SDL_SCANCODE_3:
            case SDL_SCANCODE_4:
            case SDL_SCANCODE_5:
            case SDL_SCANCODE_6:
            case SDL_SCANCODE_7:
            case SDL_SCANCODE_8:
            case SDL_SCANCODE_9:
                speed = (e->keysym.scancode - SDL_SCANCODE_1) + 1;
                break;

            case SDL_SCANCODE_F:
                toggle_fullscreen();
                break;

            case SDL_SCANCODE_L:
                break;

            case SDL_SCANCODE_S:
                break;

            default: break; // silence enum warning
        }
    }
}

static void on_key_event(const SDL_KeyboardEvent* e)
{
    const bool down = e->type == SDL_KEYDOWN;
    const bool ctrl = (e->keysym.mod & KMOD_CTRL) > 0;

    if (ctrl)
    {
        on_ctrl_key_event(e, down);

        return;
    }

    switch (e->keysym.scancode)
    {
        case SDL_SCANCODE_X:        GB_set_buttons(&gameboy, GB_BUTTON_A, down);        break;
        case SDL_SCANCODE_Z:        GB_set_buttons(&gameboy, GB_BUTTON_B, down);        break;
        case SDL_SCANCODE_RETURN:   GB_set_buttons(&gameboy, GB_BUTTON_START, down);    break;
        case SDL_SCANCODE_SPACE:    GB_set_buttons(&gameboy, GB_BUTTON_SELECT, down);   break;
        case SDL_SCANCODE_UP:       GB_set_buttons(&gameboy, GB_BUTTON_UP, down);       break;
        case SDL_SCANCODE_DOWN:     GB_set_buttons(&gameboy, GB_BUTTON_DOWN, down);     break;
        case SDL_SCANCODE_LEFT:     GB_set_buttons(&gameboy, GB_BUTTON_LEFT, down);     break;
        case SDL_SCANCODE_RIGHT:    GB_set_buttons(&gameboy, GB_BUTTON_RIGHT, down);    break;
    
        case SDL_SCANCODE_ESCAPE:
            running = false;
            break;

        default: break; // silence enum warning
    }
}

static void on_window_event(const SDL_WindowEvent* e) {
    switch (e->event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            setup_rect(e->data1, e->data2);
            break;
    }
}

static void events()
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                running = false;
                return;
        
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                on_key_event(&e.key);
                break;
        
            case SDL_WINDOWEVENT:
                on_window_event(&e.window);
                break;
        }
    } 
}

// TODO: fix starved audio buffer...
static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    (void)user;

    // using buffers because pushing 1 sample at a time seems to
    // cause popping sounds (on my chromebook).
    static uint8_t buffer1[SAMPLES * 2];
    static uint8_t buffer2[SAMPLES * 2];
    static uint8_t buffer3[SAMPLES * 2];
    static uint8_t buffer4[SAMPLES * 2];
    static size_t buffer_count = 0;

    // if speedup is enabled, skip x many samples in order to not fill the
    // audio buffer!
    if (speed > 1)
    {
        static int skipped_samples = 0;

        if (skipped_samples < speed - 1)
        {
            ++skipped_samples;
            return;
        }     

        skipped_samples = 0;   
    }

    // check if we are running ahead, if so, skip sample.
    const int audio_queue_size = SDL_GetQueuedAudioSize(audio_device);

    // enable this if vsync is enabled!
    #if 0
    if (audio_queue_size > sizeof(buffer1) * 4)
    {
        return;
    }
    #else
    (void)audio_queue_size;
    #endif

    buffer1[buffer_count + 0] = data->ch1[0];
    buffer1[buffer_count + 1] = data->ch1[1];

    buffer2[buffer_count + 0] = data->ch2[0];
    buffer2[buffer_count + 1] = data->ch2[1];

    buffer3[buffer_count + 0] = data->ch3[0];
    buffer3[buffer_count + 1] = data->ch3[1];

    buffer4[buffer_count + 0] = data->ch4[0];
    buffer4[buffer_count + 1] = data->ch4[1];

    buffer_count += 2;

    if (buffer_count == sizeof(buffer1))
    {
        buffer_count = 0;

        // this is a lazy way of mixing the channels together, along
        // with volume control.
        // it seems to sound okay for now...
        uint8_t samples[sizeof(buffer1)] = {0};

        SDL_MixAudioFormat(samples, (const uint8_t*)buffer1, AUDIO_S8, sizeof(buffer1), VOLUME);
        SDL_MixAudioFormat(samples, (const uint8_t*)buffer2, AUDIO_S8, sizeof(buffer2), VOLUME);
        SDL_MixAudioFormat(samples, (const uint8_t*)buffer3, AUDIO_S8, sizeof(buffer3), VOLUME);
        SDL_MixAudioFormat(samples, (const uint8_t*)buffer4, AUDIO_S8, sizeof(buffer4), VOLUME);

        // enable this if sync with audio
        #if 1
        while (SDL_GetQueuedAudioSize(audio_device) > (sizeof(buffer1) * 4))
        {
            SDL_Delay(AUDIO_SLEEP);
        }
        #endif

        SDL_QueueAudio(audio_device, samples, sizeof(samples));
    }
}

static uint32_t core_on_colour(void* user, enum GB_ColourCallbackType type, uint8_t r, uint8_t g, uint8_t b)
{
    (void)user;

    // SOURCE: https://near.sh/articles/video/color-emulation

    uint32_t R = 0, G = 0, B = 0;

    switch (type)
    {
        case GB_ColourCallbackType_DMG:
            R = r << 3;
            G = g << 3;
            B = b << 3;
            break;

        case GB_ColourCallbackType_GBC:
            #define min(a, b) a < b ? a : b
            R = (r * 26 + g *  4 + b *  2);
            G = (         g * 24 + b *  8);
            B = (r *  6 + g *  4 + b * 22);
            R = min(960, R) >> 2;
            G = min(960, G) >> 2;
            B = min(960, B) >> 2;
            #undef min
            break;
    }

    return SDL_MapRGB(pixel_format, R, G, B);
}

static void core_on_vblank(void* user)
{
    (void)user;

    ++frameskip_counter;

    if (frameskip_counter >= speed)
    {
        SDL_Texture* tmp = prev_texture;
        prev_texture = texture;
        texture = tmp;

        void* pixels; int pitch;

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        SDL_SetTextureBlendMode(prev_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(prev_texture, 100);

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, core_pixels, sizeof(core_pixels));
        SDL_UnlockTexture(texture);

        frameskip_counter = 0;
    }
}

static void render()
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_RenderCopy(renderer, prev_texture, NULL, &rect);
    SDL_RenderPresent(renderer);
}

static void cleanup()
{
    if (sram_data)      { munmap(sram_data, sram_size); }
    if (sram_fd != -1)  { close(sram_fd); }
    if (pixel_format)   { SDL_free(pixel_format); }
    if (audio_device)   { SDL_CloseAudioDevice(audio_device); }
    if (rom_data)       { SDL_free(rom_data); }
    if (prev_texture)   { SDL_DestroyTexture(prev_texture); }
    if (texture)        { SDL_DestroyTexture(texture); }
    if (renderer)       { SDL_DestroyRenderer(renderer); }
    if (window)         { SDL_DestroyWindow(window); }

    SDL_Quit();
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        goto fail;
    }

    // enable to record audio
    #if 0
        SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    #endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        goto fail;
    }

    window = SDL_CreateWindow("TotalGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        goto fail;
    }

    // this doesn't seem to work on chromebook...
    SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);

    // save the window pixel format, we will use this to create texure
    // with the native window format so that sdl does not have to do
    // any converting behind the scenes.
    // also, this format will be used for setting the dmg palette as well
    // as the gbc colours.
    const uint32_t pixel_format_enum = SDL_GetWindowPixelFormat(window);
    pixel_format = SDL_AllocFormat(pixel_format_enum);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    prev_texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if (!texture || !prev_texture)
    {
        goto fail;
    }

    setup_rect(WIDTH * scale, HEIGHT * scale);

    const SDL_AudioSpec wanted =
    {
        .freq = SDL_AUDIO_FREQ,
        .format = AUDIO_S8,
        .channels = 2,
        .silence = 0,
        .samples = SAMPLES,
        .padding = 0,
        .size = 0,
        .callback = NULL,
        .userdata = NULL,
    };

    SDL_AudioSpec aspec_got = {0};

    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &aspec_got, 0);

    if (audio_device == 0)
    {
        goto fail;
    }

    SDL_PauseAudioDevice(audio_device, 0);

    if (!GB_init(&gameboy))
    {
        goto fail;
    }

    GB_set_apu_callback(&gameboy, core_on_apu, NULL, GB_AUDIO_FREQ);
    GB_set_vblank_callback(&gameboy, core_on_vblank, NULL);
    GB_set_colour_callback(&gameboy, core_on_colour, NULL);
    GB_set_pixels(&gameboy, core_pixels, GB_SCREEN_WIDTH, 32);

    rom_data = (uint8_t*)SDL_LoadFile(argv[1], &rom_size);

    if (!rom_data)
    {
        goto fail;
    }

    struct GB_RomInfo rom_info = {0};

    if (!GB_get_rom_info(rom_data, rom_size, &rom_info))
    {
        goto fail;
    }

    if (rom_info.ram_size > 0)
    {
        int flags = 0;

        if (rom_info.flags & MBC_FLAGS_BATTERY)
        {
            char sram_path[0x304] = {0};

            const char* ext = strrchr(argv[1], '.');

            if (!ext)
            {
                goto fail;
            }

            strncat(sram_path, argv[1], ext - argv[1]);
            strcat(sram_path, ".sav");

            flags = MAP_SHARED;

            sram_fd = open(sram_path, O_RDWR | O_CREAT, 0644);

            if (sram_fd == -1)
            {
                perror("failed to open sram");
                goto fail;
            }

            struct stat s = {0};

            if (fstat(sram_fd, &s) == -1)
            {
                perror("failed to stat sram");
                goto fail;
            }

            if (s.st_size < rom_info.ram_size)
            {
                char page[1024] = {0};
                
                for (size_t i = 0; i < rom_info.ram_size; i += sizeof(page))
                {
                    int size = sizeof(page) > rom_info.ram_size-i ? rom_info.ram_size-i : sizeof(page);
                    write(sram_fd, page, size);
                }
            }  
        }
        else
        {
            flags = MAP_PRIVATE | MAP_ANONYMOUS;
        }

        sram_data = (uint8_t*)mmap(NULL, rom_info.ram_size, PROT_READ | PROT_WRITE, flags, sram_fd, 0);
    
        if (sram_data == MAP_FAILED)
        {
            perror("failed to mmap sram");
            goto fail;
        }

        sram_size = rom_info.ram_size;

        GB_set_sram(&gameboy, sram_data, rom_info.ram_size);
    }

    #if GB_ZROM
        struct Zrom zrom = {0};
        zrom_init(&zrom, &gameboy, zrom_mem_pool, sizeof(zrom_mem_pool));
        if (!zrom_loadrom_compressed(&zrom, rom_data, rom_size))
        {
            goto fail;
        }
    #else
        if (!GB_loadrom(&gameboy, rom_data, rom_size))
        {
            printf("failed to loadrom\n");
            goto fail;
        }
    #endif

    while (running)
    {
        events();
        run();
        render();
    }

    cleanup();

    return 0;

fail:
    printf("fail\n");
    printf("%s\n", SDL_GetError());
    cleanup();

    return -1;
}
