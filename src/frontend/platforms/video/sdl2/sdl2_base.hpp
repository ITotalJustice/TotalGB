#pragma once

#ifdef MGB_SDL2_VIDEO

#include "frontend/platforms/video/interface.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

#include <map>


namespace mgb::platform::video::sdl2::base {

struct JoystickCtx {
    SDL_Joystick *ptr;
    SDL_JoystickID id;
};

struct ControllerCtx {
    SDL_GameController *ptr;
    SDL_JoystickID id;
};

class SDL2 : public Interface {
public:
    using Interface::Interface;
    virtual ~SDL2();

    auto PollEvents() -> void override;
    auto UpdateGameTexture(GameTextureData data) -> void override;
    auto ToggleFullscreen() -> void override;
	auto SetWindowName(const std::string& name) -> void override;

protected:
    auto SetupSDL2(const VideoInfo& vid_info, const GameTextureInfo& game_info, uint32_t win_flags) -> bool;
    auto DeinitSDL2() -> void;

protected:
    SDL_Window* window{nullptr};

private:
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
    std::vector<ControllerCtx> controllers{};
    std::vector<JoystickCtx> joysticks{};
    std::vector<SDL_Haptic*> rumble_controllers{nullptr};

	std::multimap<int, Action> key_action_map;
	// this should be a vector as multiple controllers
	// can be connected at once!
	std::multimap<SDL_GameControllerButton, Action> controller_action_map;
};

} // namespace mgb::platform::video::sdl2::base

#endif // MGB_SDL2_VIDEO
