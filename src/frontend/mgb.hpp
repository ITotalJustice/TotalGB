#include <cstdint>
#include <array>
#include <memory>
#include <vector>

#include <SDL2/SDL.h>

extern "C" {
struct GB_Data;
}

namespace mgb {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

struct App {
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

} // namespace mgb
