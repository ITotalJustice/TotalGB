#include "frontend/platforms/video/sdl2/sdl2_base.hpp"

#ifdef MGB_SDL2_VIDEO

#include <cassert>
#include <cstring>
#include <cmath>
#include <array>
#include <map>
#include <optional>


namespace mgb::platform::video::sdl2::base {

// todo: replace these templates with auto once using c++20!
template <typename CB, typename BC, typename ID>
auto OnTouchUp(const CB& callbacks, BC& button_cache, ID Id) {
    // check if we have this Id
    const auto range = button_cache.equal_range(Id);
    
    // nothing found...
    if (!std::distance(range.first, range.second)) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        callbacks.OnAction(it->second, false);
    }

    button_cache.erase(Id);
}

template <typename CB, typename TB, typename BC, typename ID>
auto OnTouchDown(const CB& callbacks, const TB& touch_buttons, BC& button_cache, ID Id, int x, int y) {
    // find out if its in range...
    const auto is_in_range = [&touch_buttons, x, y]() -> std::optional<SDL2::TouchButton::Type> {
        for (auto& button : touch_buttons) {
            if (x >= button.x && x <= (button.x + button.w)) {
                if (y >= button.y && y <= (button.y + button.h)) {
                    return button.type;
                }
            }
        }

        return {};
    };

    static const auto touch_action_map = std::multimap<SDL2::TouchButton::Type, Action> {
        { SDL2::TouchButton::Type::A, Action::GAME_A },
        { SDL2::TouchButton::Type::B, Action::GAME_B },
        { SDL2::TouchButton::Type::START, Action::GAME_START },
        { SDL2::TouchButton::Type::SELECT, Action::GAME_SELECT },
        { SDL2::TouchButton::Type::UP, Action::GAME_UP },
        { SDL2::TouchButton::Type::DOWN, Action::GAME_DOWN },
        { SDL2::TouchButton::Type::LEFT, Action::GAME_LEFT },
        { SDL2::TouchButton::Type::RIGHT, Action::GAME_RIGHT },

        { SDL2::TouchButton::Type::A, Action::UI_SELECT },
        { SDL2::TouchButton::Type::B, Action::UI_BACK },
        { SDL2::TouchButton::Type::START, Action::UI_SELECT },
        { SDL2::TouchButton::Type::UP, Action::UI_UP },
        { SDL2::TouchButton::Type::DOWN, Action::UI_DOWN },
        { SDL2::TouchButton::Type::LEFT, Action::UI_LEFT },
        { SDL2::TouchButton::Type::RIGHT, Action::UI_RIGHT },
        
        // { SDL2::TouchButton::Type::OPTIONS, Action::OPTIONS },
    };

    // check that the button press maps to a texture coord
    const auto type_optional = is_in_range();
    if (!type_optional) {
        return;
    }

    const auto range = touch_action_map.equal_range(type_optional.value());
    
    // nothing found...
    if (!std::distance(range.first, range.second)) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        button_cache.insert({Id, it->second});
        callbacks.OnAction(it->second, true);
    }
}

auto SDL2::TouchButton::Reset() -> void {
    this->x = this->y = this->w = this->h = -8000;
}

auto SDL2::TouchButton::ToRect() const -> SDL_Rect {
    return { x, y, w, h };
}

SDL2::~SDL2() {
}

auto SDL2::SetupSDL2(const VideoInfo& vid_info, const GameTextureInfo& game_info, uint32_t win_flags) -> bool {
#ifndef NDEBUG
    // enable trace logging of sdl
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
#endif // NDEBUG

    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        printf("\n[SDL_VIDEO_ERROR] %s\n\n", SDL_GetError());
        return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
        printf("\n[SDL_JOYSTICK_ERROR] %s\n\n", SDL_GetError());
        return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) {
        printf("\n[SDL_GAMECONTROLLER_ERROR] %s\n\n", SDL_GetError());
        return false;
    }

    this->window = SDL_CreateWindow(
        vid_info.name.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        vid_info.w, vid_info.h, win_flags
    );

    if (!this->window) {
        printf("[SDL2] failed to create window %s\n", SDL_GetError());
        return false;
    }

    // try and load window icon, it's okay if it fails
    {
        constexpr auto icon_path = "res/icon.bmp";
        auto surface = SDL_LoadBMP(icon_path);

        if (surface) {
            SDL_SetWindowIcon(this->window, surface);
            SDL_FreeSurface(surface);
        }
        else {
            printf("[SDL2] failed to load window icon %s\n", SDL_GetError());
        }
    }

    SDL_SetWindowMinimumSize(this->window, 160, 144);

    // set the size of the buffered pixels
    this->game_pixels.resize(game_info.w * game_info.h);

    // setup keymap
    this->key_action_map = std::multimap<int, Action>{
        { SDLK_x, Action::GAME_A },
        { SDLK_z, Action::GAME_B },
        { SDLK_RETURN, Action::GAME_START },
        { SDLK_SPACE, Action::GAME_SELECT },
        { SDLK_DOWN, Action::GAME_DOWN },
        { SDLK_UP, Action::GAME_UP },
        { SDLK_LEFT, Action::GAME_LEFT },
        { SDLK_RIGHT, Action::GAME_RIGHT },

        { SDLK_SPACE, Action::UI_SELECT },
        { SDLK_RETURN, Action::UI_BACK },
        { SDLK_DOWN, Action::UI_DOWN },
        { SDLK_UP, Action::UI_UP },
        { SDLK_LEFT, Action::UI_LEFT },
        { SDLK_RIGHT, Action::UI_RIGHT },

        { SDLK_o, Action::SC_FILE_PICKER },
        { SDLK_f, Action::SC_FULLSCREEN },
    };

    this->controller_action_map = std::multimap<SDL_GameControllerButton, Action>{
        { SDL_CONTROLLER_BUTTON_A, Action::GAME_A },
        { SDL_CONTROLLER_BUTTON_B, Action::GAME_B },
        // { SDL_CONTROLLER_BUTTON_Y, },
        // { SDL_CONTROLLER_BUTTON_X, },
        { SDL_CONTROLLER_BUTTON_BACK, Action::GAME_SELECT },
        { SDL_CONTROLLER_BUTTON_START, Action::GAME_START },
        { SDL_CONTROLLER_BUTTON_DPAD_UP, Action::GAME_UP },
        { SDL_CONTROLLER_BUTTON_DPAD_DOWN, Action::GAME_DOWN },
        { SDL_CONTROLLER_BUTTON_DPAD_LEFT, Action::GAME_LEFT },
        { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Action::GAME_RIGHT },
        // { SDL_CONTROLLER_BUTTON_LEFTSTICK, },
        // { SDL_CONTROLLER_BUTTON_RIGHTSTICK, },

        // { SDL_CONTROLLER_BUTTON_GUIDE, },
    };

    constexpr auto mapping_path = "res/mappings/gamecontrollerdb.txt";

    // try and load the controller config, doesn't matter much
    // if this fails currently, though warn if it does!
    if (auto r = SDL_GameControllerAddMappingsFromFile(mapping_path); r == -1) {
        printf("\n[SDL2] failed to load mapping file: %s\n", mapping_path);
    }
    else {
        printf("\n[SDL2] loaded to load mapping file: %d\n", r);
    }

    // todo: scan for controllers here as sdl doesn't seem
    // to pick them up if they are already connected before init...
    auto num_joy = SDL_NumJoysticks();
    printf("[SDL2] joysticks found: %d\n", num_joy);
    
    for(auto i = 0; i < num_joy; i++) {
        auto joystick = SDL_JoystickOpen(i);
        printf("\t%s\n", SDL_JoystickName(joystick));
    }

    return true;
}

auto SDL2::DeinitSDL2() -> void {
    if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
        for (auto &p : this->joysticks) {
            SDL_JoystickClose(p.ptr);
            p.ptr = nullptr;
        }

        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }

    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        for (auto &p : this->controllers) {
            SDL_GameControllerClose(p.ptr);
            p.ptr = nullptr;
        }

        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (this->window) {
            SDL_DestroyWindow(this->window);
        }

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
}

auto SDL2::UpdateGameTexture(GameTextureData data) -> void {
    std::memcpy(
        this->game_pixels.data(),
        data.pixels,
        data.w * data.h * sizeof(uint16_t)
    );
}

auto SDL2::ToggleFullscreen() -> void {
    if (auto flags = SDL_GetWindowFlags(this->window); flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(this->window, 0);
    }
    else {
        SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
    }
}

auto SDL2::SetWindowName(const std::string& name) -> void {
    SDL_SetWindowTitle(this->window, name.c_str());
}

auto SDL2::HasController(int which) const -> bool {
    for (auto &p : this->controllers) {
        if (p.id == which) {
            return true;
        }
    }
    return false;
}

auto SDL2::AddController(int index) -> bool {
    auto controller = SDL_GameControllerOpen(index);
    if (!controller) {
        printf("Failed to open controller from index %d\n", index);
        return false;
    }

    ControllerCtx ctx;
    ctx.ptr = controller;
    ctx.id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
    this->controllers.push_back(std::move(ctx));

    return true;
}

auto SDL2::PollEvents() -> void {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                this->OnQuitEvent(e.quit);
                break;
            
            case SDL_DROPFILE: case SDL_DROPTEXT: case SDL_DROPBEGIN: case SDL_DROPCOMPLETE:
                this->OnDropEvent(e.drop);
                break;

#if SDL_VERSION_ATLEAST(2, 0, 9)
            case SDL_DISPLAYEVENT:
                this->OnDisplayEvent(e.display);
                break;
#endif // SDL_VERSION_ATLEAST(2, 0, 9)

            case SDL_WINDOWEVENT:
                this->OnWindowEvent(e.window);
                break;

#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_AUDIODEVICEADDED: case SDL_AUDIODEVICEREMOVED:
                this->OnAudioDeviceEvent(e.adevice);
                break;
#endif // SDL_VERSION_ATLEAST(2, 0, 4)

            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                this->OnMouseButtonEvent(e.button);
                break;

            case SDL_MOUSEMOTION:
                this->OnMouseMotionEvent(e.motion);
                break;

            case SDL_MOUSEWHEEL:
                this->OnMouseWheelEvent(e.wheel);
                break;

            case SDL_KEYDOWN: case SDL_KEYUP:
                this->OnKeyEvent(e.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP:
                this->OnControllerButtonEvent(e.cbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                this->OnControllerAxisEvent(e.caxis);
                break;

            case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP:
                this->OnJoypadButtonEvent(e.jbutton);
                break;

            case SDL_JOYAXISMOTION:
                this->OnJoypadAxisEvent(e.jaxis);
                break;

            case SDL_JOYHATMOTION:
                this->OnJoypadHatEvent(e.jhat);
                break;

            case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
                this->OnJoypadDeviceEvent(e.jdevice);
                break;

            case SDL_CONTROLLERDEVICEADDED: case SDL_CONTROLLERDEVICEREMOVED:
                this->OnControllerDeviceEvent(e.cdevice);
                break;

            case SDL_FINGERDOWN: case SDL_FINGERUP:
                this->OnTouchEvent(e.tfinger);
                break;

            case SDL_TEXTEDITING:
                this->OnTextEditEvent(e.edit);
                break;

            case SDL_TEXTINPUT:
                this->OnTextInputEvent(e.text);
                break;

            case SDL_SYSWMEVENT:
                this->OnSysWMEvent(e.syswm);
                break;

            case SDL_USEREVENT:
                this->OnUserEvent(e.user);
                break;
        }
    }
}

auto SDL2::OnQuitEvent(const SDL_QuitEvent& e) -> void {
    printf("quit request at %u\n", e.timestamp);
    this->callback.OnQuit();
}

auto SDL2::OnDropEvent(SDL_DropEvent& e) -> void {
    switch (e.type) {
        case SDL_DROPFILE:
            if (e.file != NULL) {
                this->callback.OnFileDrop(e.file);
                SDL_free(e.file);
            }
            break;
        
        case SDL_DROPTEXT:
            break;

        case SDL_DROPBEGIN:
            break;

        case SDL_DROPCOMPLETE:
            break;
    }
}

#if SDL_VERSION_ATLEAST(2, 0, 4)
auto SDL2::OnAudioDeviceEvent(const SDL_AudioDeviceEvent&) -> void {
    printf("[SDL2] SDL_AudioDeviceEvent!\n");
}
#endif // SDL_VERSION_ATLEAST(2, 0, 4)

auto SDL2::OnWindowEvent(const SDL_WindowEvent& e) -> void {
    switch (e.event) {
        case SDL_WINDOWEVENT_EXPOSED:
            // this->ResizeScreen();
            break;

        case SDL_WINDOWEVENT_RESIZED:
            // this->ResizeScreen();
            break;

        case SDL_WINDOWEVENT_RESTORED:
            // this->ResizeScreen();
            break;

        case SDL_WINDOWEVENT_CLOSE:
            break;
    }
}

#if SDL_VERSION_ATLEAST(2, 0, 9)
auto SDL2::OnDisplayEvent(const SDL_DisplayEvent& e) -> void {
   switch (e.event) {
       case SDL_DISPLAYEVENT_NONE:
           break;

       case SDL_DISPLAYEVENT_ORIENTATION:
           break;
   }
}
#endif


auto SDL2::OnMouseButtonEvent(const SDL_MouseButtonEvent& e) -> void {
    // we already handle touch events...
    if (e.which == SDL_TOUCH_MOUSEID) {
        return;
    }

    // TODO: currently, touchpad tap is broken because it goes down
    // and up on the same frame, so no button press actually happens.
    // this is actually a bug with all inputs as well, but only
    // noticable with touchpad.

    // to fix this, i could check if the button state has already changed this
    // frame, and if so, ignore further updates.
    // however, the further updates need to be saved for next frame and then handled
    // because if a button is say, released, and we ignored it, then the button will
    // be forever held until it is pressed and released again!

    // another issue this will cause is that if on the next frame, theres a
    // new button event fired, that already has a buffered event from the last frame
    // i think the old event should be ignored, and only the new one should be handled.

    // an alternitive way to fix this is to poll inputs more frequent than once
    // per frame.
    // this is probably the most ideal fix, though, this messes up the event system
    // as we dont want to poll events that often as they can be
    // costly or drastically change a setting that is expected to be called
    // at the end of a frame...
    // 
    if (e.type == SDL_MOUSEBUTTONUP) {
        OnTouchUp(
            this->callback, this->mouse_down_buttons,
            e.which
        );
    }

    else if (e.type == SDL_MOUSEBUTTONDOWN) {
        OnTouchDown(
            this->callback, this->touch_buttons, this->mouse_down_buttons,
            e.which, e.x, e.y
        );
    }
}

auto SDL2::OnMouseMotionEvent(const SDL_MouseMotionEvent&) -> void {
    // this gets noisy
    // printf("[SDL2] SDL_MouseMotionEvent!\n");
}

auto SDL2::OnMouseWheelEvent(const SDL_MouseWheelEvent&) -> void {
    // this gets noisy
    // printf("[SDL2] SDL_MouseWheelEvent!\n");
}

auto SDL2::OnKeyEvent(const SDL_KeyboardEvent& e) -> void {
    const auto kdown = e.type == SDL_KEYDOWN;

    const auto range = this->key_action_map.equal_range(e.keysym.sym);
    
    // nothing found...
    if (!std::distance(range.first, range.second)) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        this->callback.OnAction(it->second, kdown);
    }
}

auto SDL2::OnJoypadAxisEvent(const SDL_JoyAxisEvent&) -> void {

}

auto SDL2::OnJoypadButtonEvent(const SDL_JoyButtonEvent&) -> void {

}

auto SDL2::OnJoypadHatEvent(const SDL_JoyHatEvent&) -> void {

}

auto SDL2::OnJoypadDeviceEvent(const SDL_JoyDeviceEvent&) -> void {
    printf("[SDL2] joypad device event!\n");
}

auto SDL2::OnControllerAxisEvent(const SDL_ControllerAxisEvent& e) -> void {
    constexpr auto deadzone = 8000;

    constexpr auto left     = -deadzone;
    constexpr auto right    = +deadzone;
    constexpr auto up       = -deadzone;
    constexpr auto down     = +deadzone;

    switch (e.axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: case SDL_CONTROLLER_AXIS_RIGHTX:
            if (e.value < left) {
                this->callback.OnAction(Action::GAME_LEFT, true);
            }
            else if (e.value > right) {
                this->callback.OnAction(Action::GAME_RIGHT, true);
            }
            else {
                this->callback.OnAction(Action::GAME_LEFT, false);
                this->callback.OnAction(Action::GAME_RIGHT, false);
            }
            break;

        case SDL_CONTROLLER_AXIS_LEFTY: case SDL_CONTROLLER_AXIS_RIGHTY:
            if (e.value < up) {
                this->callback.OnAction(Action::GAME_UP, true);
            }
            else if (e.value > down) {
                this->callback.OnAction(Action::GAME_UP, false);
            }
            else {
                this->callback.OnAction(Action::GAME_UP, false);
                this->callback.OnAction(Action::GAME_DOWN, false);
            }
            break;
    }
}

auto SDL2::OnControllerButtonEvent(const SDL_ControllerButtonEvent& e) -> void {
    assert(e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP);
    
    if (!this->HasController(e.which)) {
        std::printf("[SDL2] unknown controller unkown cbutton %d\n", e.button);
        return;
    }

    const auto kdown = e.type == SDL_CONTROLLERBUTTONDOWN;

    // sdl2 has the entry as a u8, so i cast it here.
    const auto button = static_cast<SDL_GameControllerButton>(e.button);
    
    const auto range = this->controller_action_map.equal_range(button);
    
    // nothing found...
    if (!std::distance(range.first, range.second)) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        this->callback.OnAction(it->second, kdown);
    }
}

auto SDL2::OnControllerDeviceEvent(const SDL_ControllerDeviceEvent& e) -> void {
    if (e.type == SDL_CONTROLLERDEVICEADDED) {
        printf("[SDL2] CONTROLLER ADDED");
        this->AddController(e.which);
    }
    else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
        printf("[SDL2] CONTROLLER REMOVED");
        int i = 0;
        for (auto& p : this->controllers) {
            if (p.id == e.which) {
                SDL_GameControllerClose(p.ptr);
                p.ptr = nullptr;
                this->controllers.erase(this->controllers.begin() + i);
                break;
            }
            i++;
        }
    }
}

auto SDL2::OnTouchEvent(const SDL_TouchFingerEvent& e) -> void {
    if (e.type == SDL_FINGERUP) {
        OnTouchUp(
            this->callback, this->touch_down_buttons,
            e.fingerId
        );
    }

    else if (e.type == SDL_FINGERDOWN) {
        // we need to un-normalise x, y
        const int x = e.x * 640;
        const int y = e.y * 576;

        OnTouchDown(
            this->callback, this->touch_buttons, this->touch_down_buttons,
            e.fingerId, x, y
        );
    }
}

auto SDL2::OnMultiGestureEvent(const SDL_MultiGestureEvent&) -> void {
    printf("[SDL2] SDL_MultiGestureEvent!\n");
}

auto SDL2::OnDollarGestureEvent(const SDL_DollarGestureEvent&) -> void {
    printf("[SDL2] SDL_DollarGestureEvent!\n");
}

auto SDL2::OnTextEditEvent(const SDL_TextEditingEvent&) -> void {
    printf("[SDL2] SDL_TextEditingEvent!\n");
}

auto SDL2::OnTextInputEvent(const SDL_TextInputEvent&) -> void {
    // this gets noisy
    // printf("[SDL2] SDL_TextInputEvent!\n");
}

auto SDL2::OnSysWMEvent(const SDL_SysWMEvent&) -> void {
    printf("[SDL2] SDL_SysWMEvent!\n");
}

auto SDL2::OnUserEvent(SDL_UserEvent&) -> void {
    printf("[SDL2] user event!\n");
}

} // namespace mgb::platform::video::sdl2::base {

#endif // MGB_SDL2_VIDEO

