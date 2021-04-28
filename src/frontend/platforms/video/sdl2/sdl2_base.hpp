#pragma once

#ifdef MGB_SDL2_VIDEO

#include "frontend/platforms/video/interface.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

#include <array>
#include <vector>
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
    struct TouchButton {
        enum class Type {
            A,
            B,
            START,
            SELECT,
            UP,
            DOWN,
            LEFT,
            RIGHT,
            OPTIONS,
        };

        ~TouchButton();

        // sets the coords to -8000
        auto Reset() -> void;
        
        SDL_Surface* surface{};

        Type type{};

        // have the buttons be offscreen by default so that
        // they don't trigger if touch is not supported
        SDL_Rect rect{-8000, -8000, -8000, -8000};
    };
    
public:
    using Interface::Interface;
    virtual ~SDL2();

    auto PollEvents() -> void override;
    auto ToggleFullscreen() -> void override;
	auto SetWindowName(const std::string& name) -> void override;

    auto GetWindow() { return this->window; }
    
protected:
    auto SetupSDL2(const VideoInfo& vid_info, const GameTextureInfo& game_info, uint32_t win_flags) -> bool;
    auto DeinitSDL2() -> void;
    auto LoadButtonSurfaces() -> bool;

    auto OnWindowResize() -> void;

protected:
    SDL_Window* window{};
    std::array<TouchButton, 9> touch_buttons;

    SDL_Rect texture_rect{};
    SDL_Rect window_rect{};

private:
    auto ResizeButtons(int win_w, int win_h, int scale) -> void;

    auto HasController(int which) const -> bool;
    auto AddController(int index) -> bool;

    auto OnQuitEvent(const SDL_QuitEvent& e) -> void;
    auto OnDropEvent(SDL_DropEvent& e) -> void; // need to free text.
    auto OnWindowEvent(const SDL_WindowEvent& e) -> void;
#if SDL_VERSION_ATLEAST(2, 0, 4)
    auto OnAudioDeviceEvent(const SDL_AudioDeviceEvent& e) -> void;
#endif // SDL_VERSION_ATLEAST(2, 0, 4)
#if SDL_VERSION_ATLEAST(2, 0, 9)
    auto OnDisplayEvent(const SDL_DisplayEvent& e) -> void;
#endif // SDL_VERSION_ATLEAST(2, 0, 9)
    auto OnMouseButtonEvent(const SDL_MouseButtonEvent& e) -> void;
    auto OnMouseMotionEvent(const SDL_MouseMotionEvent& e) -> void;
    auto OnMouseWheelEvent(const SDL_MouseWheelEvent& e) -> void;
    auto OnKeyEvent(const SDL_KeyboardEvent& e) -> void;
    auto OnJoypadAxisEvent(const SDL_JoyAxisEvent& e) -> void;
    auto OnJoypadButtonEvent(const SDL_JoyButtonEvent& e) -> void;
    auto OnJoypadHatEvent(const SDL_JoyHatEvent& e) -> void;
    auto OnJoypadDeviceEvent(const SDL_JoyDeviceEvent& e) -> void;
    auto OnControllerAxisEvent(const SDL_ControllerAxisEvent& e) -> void;
    auto OnControllerButtonEvent(const SDL_ControllerButtonEvent& e) -> void;
    auto OnControllerDeviceEvent(const SDL_ControllerDeviceEvent& e) -> void;
    auto OnTouchEvent(const SDL_TouchFingerEvent& e) -> void;
    auto OnMultiGestureEvent(const SDL_MultiGestureEvent& e) -> void;
    auto OnDollarGestureEvent(const SDL_DollarGestureEvent& e) -> void;
    auto OnTextEditEvent(const SDL_TextEditingEvent& e) -> void;
    auto OnTextInputEvent(const SDL_TextInputEvent& e) -> void;
    auto OnSysWMEvent(const SDL_SysWMEvent& e) -> void;
    auto OnUserEvent(SDL_UserEvent& e) -> void; // might need to free.

private:
    std::vector<ControllerCtx> controllers{};
    std::vector<JoystickCtx> joysticks{};
    std::vector<SDL_Haptic*> rumble_controllers{nullptr};

    // these save the instance of a button / mouse click successfully
    // pressing a button.
    // as the mouse / finger can be dragged away from button after press
    // it is impossible to then release the action.
    // so we need to keep track of the button pressed, using it's matching ID
    std::multimap<std::uint32_t, std::pair<TouchButton::Type, Action>> mouse_down_buttons;
    std::multimap<SDL_FingerID, std::pair<TouchButton::Type, Action>> touch_down_buttons;

	std::multimap<int, Action> key_action_map;
    std::multimap<int, Action> touch_action_map;
	// this should be a vector as multiple controllers
	// can be connected at once!
	std::multimap<SDL_GameControllerButton, Action> controller_action_map;
};

} // namespace mgb::platform::video::sdl2::base

#endif // MGB_SDL2_VIDEO
