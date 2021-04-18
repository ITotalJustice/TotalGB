#include "frontend/platforms/video/sdl2/sdl2_video.hpp"
#include "core/gb.h"

#ifdef MGB_SDL2_VIDEO

#include <cassert>
#include <cstring>
#include <array>


namespace mgb::platform::video::sdl2 {


struct KeyMapEntry {
    int key;
    GB_Button button;
};

constexpr std::array<KeyMapEntry, 8> key_map{{
    {SDLK_x, GB_BUTTON_A},
    {SDLK_z, GB_BUTTON_B},
    {SDLK_RETURN, GB_BUTTON_START},
    {SDLK_SPACE, GB_BUTTON_SELECT},
    {SDLK_DOWN, GB_BUTTON_DOWN},
    {SDLK_UP, GB_BUTTON_UP},
    {SDLK_LEFT, GB_BUTTON_LEFT},
    {SDLK_RIGHT, GB_BUTTON_RIGHT},
}};


BaseSDL2::~BaseSDL2() {
}

auto BaseSDL2::SetupSDL2(const VideoInfo& vid_info, const GameTextureInfo& game_info, uint32_t win_flags) -> bool {
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

    // set the size of the buffered pixels
    this->game_pixels.resize(game_info.w * game_info.h);

    return true;
}

auto BaseSDL2::DeinitSDL2() -> void {
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

auto BaseSDL2::UpdateGameTexture(GameTextureData data) -> void {
    std::memcpy(
        this->game_pixels.data(),
        data.pixels,
        data.w * data.h * sizeof(uint16_t)
    );
}

auto BaseSDL2::HasController(int which) const -> bool {
    for (auto &p : this->controllers) {
        if (p.id == which) {
            return true;
        }
    }
    return false;
}

auto BaseSDL2::AddController(int index) -> bool {
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

auto BaseSDL2::PollEvents() -> void {
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
            
            case SDL_USEREVENT:
                this->OnUserEvent(e.user);
                break;
        }
    }
}

auto BaseSDL2::OnQuitEvent(const SDL_QuitEvent& e) -> void {
    printf("quit request at %u\n", e.timestamp);
    this->callback.OnQuit();
}

auto BaseSDL2::OnDropEvent(SDL_DropEvent& e) -> void {
    switch (e.type) {
        case SDL_DROPFILE:
            if (e.file != NULL) {
                this->callback.LoadRom(e.file);
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

auto BaseSDL2::OnAudioEvent(const SDL_AudioDeviceEvent&) -> void {

}

auto BaseSDL2::OnWindowEvent(const SDL_WindowEvent& e) -> void {
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
auto BaseSDL2::OnDisplayEvent(const SDL_DisplayEvent& e) -> void {
   switch (e.event) {
       case SDL_DISPLAYEVENT_NONE:
           break;

       case SDL_DISPLAYEVENT_ORIENTATION:
           break;
   }
}
#endif

auto BaseSDL2::OnKeyEvent(const SDL_KeyboardEvent& e) -> void {
    const auto kdown = e.type == SDL_KEYDOWN;

    // first check if any of the mapped keys were pressed...
    for (auto [key, button] : key_map) {
        if (key == e.keysym.sym) {
            GB_set_buttons(this->callback.GetCore(), button, kdown);
            return;
        }
    }

    // usually best to respond once the key is released
    if (kdown) {
        return;
    }

    // otherwise, check if its a valid key
    switch (e.keysym.sym) {
        case SDLK_o:
            this->callback.FilePicker();
            break;

    // these are hotkeys to toggle layers of the gb core
    // such as bg, win, obj...
        case SDLK_0:
            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_ALL);
            break;

        case SDLK_1:
            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_BG);
            break;

        case SDLK_2:
            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_WIN);
            break;

        case SDLK_3:
            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_OBJ);
            break;

    // these are for savestates
        case SDLK_F1:
            this->callback.SaveState();
            break;

        case SDLK_F2:
            this->callback.LoadState();
            break;

    // these are for debugging
        case SDLK_l: {
            static bool cpu_log = false;
            cpu_log = !cpu_log;
            GB_cpu_enable_log(cpu_log);
        } break;

    }
}

auto BaseSDL2::OnJoypadAxisEvent(const SDL_JoyAxisEvent&) -> void {

}

auto BaseSDL2::OnJoypadButtonEvent(const SDL_JoyButtonEvent&) -> void {

}

auto BaseSDL2::OnJoypadHatEvent(const SDL_JoyHatEvent&) -> void {

}

auto BaseSDL2::OnJoypadDeviceEvent(const SDL_JoyDeviceEvent&) -> void {

}

auto BaseSDL2::OnControllerAxisEvent(const SDL_ControllerAxisEvent&) -> void {

}

auto BaseSDL2::OnControllerButtonEvent(const SDL_ControllerButtonEvent& e) -> void {
    assert(e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP);
    
    if (!this->HasController(e.which)) {
        // log::log("unkown controller unkown cbutton" + std::to_string(e.button));
        return;
    }

    const bool down = e.type == SDL_CONTROLLERBUTTONDOWN;

    switch (e.button) {
        case SDL_CONTROLLER_BUTTON_A: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_Y: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_B: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_X: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_BACK: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_SELECT, down); break;
        case SDL_CONTROLLER_BUTTON_START: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_START, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_UP, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_DOWN, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_LEFT, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: GB_set_buttons(this->callback.GetCore(), GB_BUTTON_RIGHT, down); break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: break;
        case SDL_CONTROLLER_BUTTON_GUIDE: break; // home
        default:
            printf("unkown cbutton %u\n", e.button);
            break;
    }
}

auto BaseSDL2::OnControllerDeviceEvent(const SDL_ControllerDeviceEvent& e) -> void {
    if (e.type == SDL_CONTROLLERDEVICEADDED) {
        printf("CONTROLLER ADDED");
        this->AddController(e.which);
    } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
        printf("CONTROLLER REMOVED");
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

auto BaseSDL2::OnTouchEvent(const SDL_TouchFingerEvent&) -> void {

}

auto BaseSDL2::OnUserEvent(SDL_UserEvent&) -> void {

}

} // namespace mgb::platform::video::sdl2 {

#endif // MGB_SDL2_VIDEO

