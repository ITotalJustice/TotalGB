#include <stdbool.h>
#include <stdint.h>
#include <gb.h>
#include <SDL.h>


#define WIDTH 160
#define HEIGHT 144

#define AUDIO_FREQ 48000

static struct GB_Core gameboy;
static uint16_t core_pixels[144][160];
static void* rom_data = NULL;
static size_t rom_size = 0;
static bool running = true;
static int scale = 2;
static int speed = 1;
static int frameskip_counter = 0;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_AudioDeviceID audio_device = 0;


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

static void resize_screen()
{
    SDL_SetWindowSize(window, WIDTH * scale, HEIGHT * scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void on_key_event(const SDL_KeyboardEvent* e)
{
    const bool down = e->type == SDL_KEYDOWN;
    const bool ctrl = (e->keysym.mod & KMOD_CTRL) > 0;

    if (ctrl && !down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_EQUALS:
            case SDL_SCANCODE_KP_PLUS:
                ++scale;
                resize_screen();
                break;

            case SDL_SCANCODE_MINUS:
            case SDL_SCANCODE_KP_PLUSMINUS:
            case SDL_SCANCODE_KP_MINUS:
                scale = scale > 0 ? scale - 1 : 1;
                resize_screen();
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

            default: break; // silence enum warning
        }

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

        case SDL_SCANCODE_I:
            GB_cpu_enable_log(down);
            break;

        default: break; // silence enum warning
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
        }
    } 
}

static void core_on_apu(void* user, struct GB_ApuCallbackData* data)
{
    while (SDL_GetQueuedAudioSize(audio_device) > (1024 * 8)) {
        SDL_Delay(1);
    }

    uint8_t buffer[2] = {0};

    SDL_MixAudioFormat(buffer, (const uint8_t*)data->ch1, AUDIO_S8, sizeof(data->ch1), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->ch2, AUDIO_S8, sizeof(data->ch2), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->ch3, AUDIO_S8, sizeof(data->ch3), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->ch4, AUDIO_S8, sizeof(data->ch4), SDL_MIX_MAXVOLUME/8);

    switch (SDL_GetAudioDeviceStatus(audio_device)) {
        case SDL_AUDIO_STOPPED:
            // std::printf("[SDL2-AUDIO] stopped\n");
            break;

        case SDL_AUDIO_PAUSED:
            // std::printf("[SDL2-AUDIO] paused\n");
            break;

        case SDL_AUDIO_PLAYING:
            SDL_QueueAudio(audio_device, buffer, sizeof(buffer));
            break;
    }

    // SDL_QueueAudio(audio_device, data->samples, sizeof(data->samples));
}

static void core_on_vblank(void* user)
{
    (void)user;

    ++frameskip_counter;

    if (frameskip_counter >= speed)
    {
        void* pixels; int pitch;

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, core_pixels, sizeof(core_pixels));
        SDL_UnlockTexture(texture);

        frameskip_counter = 0;
    }
}

static void render()
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void cleanup()
{
    if (audio_device)   { SDL_CloseAudioDevice(audio_device); }
    if (rom_data)       { SDL_free(rom_data); }
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

    if (!GB_init(&gameboy))
    {
        goto fail;
    }

    GB_set_apu_callback(&gameboy, core_on_apu, AUDIO_FREQ+256);
    GB_set_vblank_callback(&gameboy, core_on_vblank);

    rom_data = SDL_LoadFile(argv[1], &rom_size);

    if (!rom_data)
    {
        goto fail;
    }

    if (!GB_loadrom(&gameboy, rom_data, rom_size))
    {
        goto fail;
    }

    struct GB_CartName rom_name = {0};

    GB_get_rom_name(&gameboy, &rom_name);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        goto fail;
    }

    window = SDL_CreateWindow(rom_name.name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window)
    {
        goto fail;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if (!texture)
    {
        goto fail;
    }


    const SDL_AudioSpec wanted = {
        .freq = AUDIO_FREQ,
        .format = AUDIO_S8,
        .channels = 2,
        .silence = 0, // calculated
        .samples = 512, // 512 * 2 (because stereo)
        .padding = 0,
        .size = 0, // calculated
        .callback = NULL,
        .userdata = NULL,
    };

    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);

    if (audio_device == 0)
    {
        goto fail;
    }

    SDL_PauseAudioDevice(audio_device, 0);

    GB_set_pixels(&gameboy, core_pixels, GB_SCREEN_WIDTH);

    while (running)
    {
        events();
        run();
        render();
    }

    cleanup();

    return 0;

fail:
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
    cleanup();

    return -1;
}
