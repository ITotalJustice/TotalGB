#pragma once


#include "frontend/platforms/video/interface.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

namespace mgb::platform::video::sdl2 {


struct JoystickCtx {
    SDL_Joystick *ptr;
    SDL_JoystickID id;
};

struct ControllerCtx {
    SDL_GameController *ptr;
    SDL_JoystickID id;
};

class SDL2 final : public Interface {
public:
	using Interface::Interface;
	~SDL2();

	auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool override;

	auto UpdateGameTexture(GameTextureData data) -> void override;

	auto RenderDisplay() -> void override;

	auto PollEvents() -> void override;

protected:

private:
	auto ResizeScreen() -> void;

    auto HasController(int which) const -> bool;
    auto AddController(int index) -> bool;

    auto OnQuitEvent(const SDL_QuitEvent& e) -> void;
    auto OnDropEvent(SDL_DropEvent& e) -> void; // need to free text.
    auto OnAudioEvent(const SDL_AudioDeviceEvent& e) -> void;
    auto OnWindowEvent(const SDL_WindowEvent& e) -> void;
#if SDL_VERSION_ATLEAST(2, 0, 9)
    auto OnDisplayEvent(const SDL_DisplayEvent& e) -> void;
#endif // SDL_VERSION_ATLEAST(2, 0, 9)
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
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    SDL_Texture* texture{nullptr};

    std::vector<ControllerCtx> controllers{};
    std::vector<JoystickCtx> joysticks{};
    std::vector<SDL_Haptic*> rumble_controllers{nullptr};
};

} // namespace mgb::platform::video::sdl2 {
