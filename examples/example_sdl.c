#include <stdbool.h>
#include <stdint.h>
#include <gb.h>
#include <SDL.h>


enum
{
    WIDTH = 160,
    HEIGHT = 144,

    VOLUME = SDL_MIX_MAXVOLUME/16,
    SAMPLES = 1024,
    SDL_AUDIO_FREQ = 48000,
    GB_AUDIO_FREQ = SDL_AUDIO_FREQ,

    // AUDIO_SLEEP = SDL_AUDIO_FREQ / (SAMPLES * 2) / 4,
    AUDIO_SLEEP = 2,
};


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
static SDL_Rect rect = {0};

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

static void on_window_event(const SDL_WindowEvent* e) {
    switch (e->event) {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_RESTORED:
            break;

        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_HIDDEN:
            break;

        case SDL_WINDOWEVENT_MOVED:
            break;

        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            setup_rect(e->data1, e->data2);
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            break;

        case SDL_WINDOWEVENT_ENTER:
            break;

        case SDL_WINDOWEVENT_LEAVE:
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            break;

        case SDL_WINDOWEVENT_CLOSE:
            break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            break;

        case SDL_WINDOWEVENT_HIT_TEST:
            break;
#endif // SDL_VERSION_ATLEAST(2, 0, 5)
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
        static size_t skipped_samples = 0;

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
    SDL_RenderCopy(renderer, texture, NULL, &rect);
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

    GB_set_apu_callback(&gameboy, core_on_apu, GB_AUDIO_FREQ);
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

    // SDL_setenv("SDL_AUDIODRIVER", "sndio", 1);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        goto fail;
    }

    window = SDL_CreateWindow(rom_name.name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window)
    {
        goto fail;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if (!texture)
    {
        goto fail;
    }

    setup_rect(WIDTH * scale, HEIGHT * scale);

    const SDL_AudioSpec wanted = {
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

    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &aspec_got, /*SDL_AUDIO_ALLOW_ANY_CHANGE*/0);

    if (audio_device == 0)
    {
        goto fail;
    }

    printf("SDL_AUDIO_SPEC\n");
    printf("\tfreq: %u\n", aspec_got.freq);
    printf("\tformat: %u\n", aspec_got.format);
    printf("\tchannels: %u\n", aspec_got.channels);
    printf("\tsamples: %u\n", aspec_got.samples);
    printf("\tsize: %u\n", aspec_got.size);

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
