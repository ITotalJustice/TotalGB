#include "core/gb.h"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <memory>
#include <vector>
#include <fstream>

#include <SDL2/SDL.h>

namespace {
using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

struct App {
public:
    struct KeyMapEntry {
		int key;
		enum GB_Button button;
	};

	static constexpr KeyMapEntry key_map[]{
		{SDLK_x, GB_BUTTON_A},
		{SDLK_z, GB_BUTTON_B},
		{SDLK_RETURN, GB_BUTTON_START},
		{SDLK_SPACE, GB_BUTTON_SELECT},
		{SDLK_DOWN, GB_BUTTON_DOWN},
		{SDLK_UP, GB_BUTTON_UP},
		{SDLK_LEFT, GB_BUTTON_LEFT},
		{SDLK_RIGHT, GB_BUTTON_RIGHT},
	};

public:
    App();
    ~App();

    auto LoadRom(const char* path) -> bool;
    auto Loop() -> void;

private:
    auto Draw() -> void;
    auto Events() -> void;

private:
    std::unique_ptr<GB_Data> gameboy;
    std::vector<u8> rom_data;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool running{true};
};

App::App() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    // window
	this->window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	SDL_SetWindowMinimumSize(window, 160, 144);
    SDL_ShowCursor(SDL_DISABLE);

    this->gameboy = std::make_unique<GB_Data>();
    GB_init(this->gameboy.get());
}

App::~App() {
	GB_quit(this->gameboy.get());

    SDL_DestroyTexture(this->texture);
	SDL_DestroyRenderer(this->renderer);
	SDL_DestroyWindow(this->window);
	SDL_Quit();
}

auto App::LoadRom(const char* path) -> bool {
    std::ifstream fs{path, std::ios_base::binary};
    if (!fs.good()) {
        return false;
    }

    fs.seekg(0, std::ios_base::end);
    auto file_size = fs.tellg();
    fs.seekg(0, std::ios_base::beg);

    this->rom_data.resize(file_size);
    fs.read(reinterpret_cast<char*>(this->rom_data.data()), this->rom_data.size());

    GB_loadrom_data(this->gameboy.get(),
        this->rom_data.data(), this->rom_data.size(), NULL
    );

    return true;
}

auto App::Loop() -> void {
    while (this->running) {
        this->Events();
        GB_run_frame(gameboy.get());
        this->Draw();
    }
}

auto App::Draw() -> void {
    SDL_RenderClear(renderer);
    void* pixles; int pitch;

    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    memcpy(pixles, gameboy->ppu.pixles, sizeof(gameboy->ppu.pixles));
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

auto App::Events() -> void {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                this->running = false;
                break;
            
            case SDL_KEYDOWN: case SDL_KEYUP: {
                const GB_BOOL kdown = e.type == SDL_KEYDOWN;
                for (size_t i = 0; i < GB_ARR_SIZE(key_map); ++i) {
                    if (key_map[i].key == e.key.keysym.sym) {
                        GB_set_buttons(gameboy.get(), key_map[i].button, kdown);
                        break;
                    }
                }
            } break;
        }
    }
}

} // namespace

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        printf("missing rom path...\n");
        return -1;
    }

    App app;
    if (app.LoadRom(argv[1])) {
        app.Loop();
    }

    return 0;
}
