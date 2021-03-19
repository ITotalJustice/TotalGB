#include <cstdint>
#include <array>
#include <memory>
#include <vector>
#include <string>

#include <SDL2/SDL.h>

extern "C" {
struct GB_Data;
struct GB_ErrorData;
}

namespace mgb {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

struct JoystickCtx {
    SDL_Joystick *ptr;
    SDL_JoystickID id;
};

struct ControllerCtx {
    SDL_GameController *ptr;
    SDL_JoystickID id;
};

struct App {
public:
    App();
    ~App();

    auto LoadRom(const std::string& path) -> bool;
    auto Loop() -> void;

    auto OnVblankCallback() -> void;
    auto OnHblankCallback() -> void;
    auto OnDmaCallback() -> void;
    auto OnHaltCallback() -> void;
    auto OnStopCallback() -> void;
    auto OnErrorCallback(struct GB_ErrorData* data) -> void;

private:
    auto SaveGame(const std::string& path) -> void;
    auto LoadSave(const std::string& path) -> void;

    auto FilePicker() -> void;

    auto Draw() -> void;
    auto Events() -> void;

    auto ResizeScreen() -> void;

    auto HasController(int which) const -> bool;

    auto AddController(int index) -> bool;

    auto OnQuitEvent(const SDL_QuitEvent& e) -> void;
    auto OnDropEvent(SDL_DropEvent& e) -> void; // need to free text.
    auto OnAudioEvent(const SDL_AudioDeviceEvent& e) -> void;
    auto OnWindowEvent(const SDL_WindowEvent& e) -> void;
    auto OnDisplayEvent(const SDL_DisplayEvent& e) -> void;
    auto OnKeyEvent(const SDL_KeyboardEvent& e) -> void;
    auto OnJoypadAxisEvent(const SDL_JoyAxisEvent& e) -> void;
    auto OnJoypadButtonEvent(const SDL_JoyButtonEvent& e) -> void;
    auto OnJoypadHatEvent(const SDL_JoyHatEvent& e) -> void;
    auto OnJoypadDeviceEvent(const SDL_JoyDeviceEvent& e) -> void;
    auto OnControllerAxisEvent(const SDL_ControllerAxisEvent& e) -> void;
    auto OnControllerButtonEvent(const SDL_ControllerButtonEvent& e) -> void;
    auto OnControllerDeviceEvent(const SDL_ControllerDeviceEvent& e) -> void;
    auto OnTouchEvent(const SDL_TouchFingerEvent& e) -> void;
    auto OnUserEvent(SDL_UserEvent& e) -> void; // might need to free.

private:
    std::unique_ptr<GB_Data> gameboy;
    std::vector<u8> rom_data;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    std::string rom_path;

    std::vector<ControllerCtx> controllers{};
    std::vector<JoystickCtx> joysticks{};
    std::vector<SDL_Haptic*> rumble_controllers{nullptr};

    bool rom_loaded{false};
    bool running{true};
};

} // namespace mgb
