#include <cstdint>
#include <array>
#include <memory>
#include <vector>
#include <string>

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // WIN32

extern "C" {
struct GB_Data;
struct GB_ErrorData;
}

namespace mgb {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

enum class EmuRunState {
    NONE,
    SINGLE,
    DUAL
};

struct JoystickCtx {
    SDL_Joystick *ptr;
    SDL_JoystickID id;
};

struct ControllerCtx {
    SDL_GameController *ptr;
    SDL_JoystickID id;
};

// this is a quick hacky gb instance struct
struct Instance {
    std::unique_ptr<GB_Data> gameboy;
    std::vector<u8> rom_data;
    std::string rom_path;

    SDL_Texture* texture{nullptr};
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};

    bool rom_loaded{false};

    auto LoadRom(const std::string& path) -> bool;
    auto SaveGame(const std::string& path) -> void;
    auto LoadSave(const std::string& path) -> void;

    auto OnVblankCallback() -> void;
    auto OnHblankCallback() -> void;
    auto OnDmaCallback() -> void;
    auto OnHaltCallback() -> void;
    auto OnStopCallback() -> void;
    auto OnErrorCallback(struct GB_ErrorData* data) -> void;
};

struct App {
public:
    App();
    ~App();

    auto LoadRom(const std::string& path, bool dual = false) -> bool;
    auto Loop() -> void;

private:
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
    //auto OnDisplayEvent(const SDL_DisplayEvent& e) -> void;
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
    std::array<Instance, 2> emu_instances;

    std::vector<ControllerCtx> controllers{};
    std::vector<JoystickCtx> joysticks{};
    std::vector<SDL_Haptic*> rumble_controllers{nullptr};

    EmuRunState run_state{EmuRunState::NONE};

    bool running{true};
};

} // namespace mgb
