#include <stdbool.h>
#include <gb.h>
#include <SDL.h>


#define WIDTH 160
#define HEIGHT 144

static struct GB_Core gameboy;
static uint16_t core_pixels[144][160];
static void* rom_data = NULL;
static size_t rom_size = 0;
static bool running = true;
static int scale = 2;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;


static void run()
{
    GB_run_frame(&gameboy);
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
        }

        return;
    }

    switch (e->keysym.scancode)
    {
        case SDL_SCANCODE_X:        GB_set_buttons(&gameboy, GB_BUTTON_B, down);        break;
        case SDL_SCANCODE_Z:        GB_set_buttons(&gameboy, GB_BUTTON_A, down);        break;
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

static void core_on_vblank(struct GB_Core* gb, void* user)
{
    void* pixels; int pitch;

    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    memcpy(pixels, core_pixels, sizeof(core_pixels));
    SDL_UnlockTexture(texture);
}

static void render()
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return -1;
    }

    if (!GB_init(&gameboy))
    {
        return -1;
    }

    GB_set_vblank_callback(&gameboy, core_on_vblank, NULL);

    rom_data = SDL_LoadFile(argv[1], &rom_size);

    if (!rom_data)
    {
        goto fail;
    }

    if (!GB_loadrom(&gameboy, rom_data, rom_size))
    {
        return -1;
    }

    struct GB_CartName rom_name = {0};

    GB_get_rom_name(&gameboy, &rom_name);

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


    GB_set_pixels(&gameboy, core_pixels, GB_SCREEN_WIDTH);

    while (running)
    {
        events();
        run();
        render();
    }

    SDL_free(rom_data);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

fail:
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
    return -1;
}
