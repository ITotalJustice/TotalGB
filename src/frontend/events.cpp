#include "mgb.hpp"
#include "../core/gb.h"
#include "util/util.hpp"
#include "util/log.hpp"

#include <cassert>
#include <array>

namespace mgb {

struct KeyMapEntry {
    int key;
    enum GB_Button button;
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

auto App::Events() -> void {
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

auto App::OnQuitEvent(const SDL_QuitEvent& e) -> void {
    log::log("quit request at %u\n", e.timestamp);
    printf("quit request at %u\n", e.timestamp);
    this->running = false;
}

auto App::OnDropEvent(SDL_DropEvent& e) -> void {
    switch (e.type) {
        case SDL_DROPFILE:
            if (e.file != NULL) {
                this->LoadRom(e.file);
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

auto App::OnAudioEvent(const SDL_AudioDeviceEvent&) -> void {

}

auto App::OnWindowEvent(const SDL_WindowEvent& e) -> void {
    switch (e.event) {
        case SDL_WINDOWEVENT_EXPOSED:
            this->ResizeScreen();
            break;

        case SDL_WINDOWEVENT_RESIZED:
            this->ResizeScreen();
            break;

        case SDL_WINDOWEVENT_RESTORED:
            this->ResizeScreen();
            break;

        /*
        so when theres multiple windows (in sdl2) open, sdl quit will no longer
        trigger once a window is selected to be closed...which makes sense. 

        instead a window event is sent, with type close. note however that
        this event is sent even when theres only 1 window, so this event will
        trigger along with sdl_quit if its the last window
        */
        case SDL_WINDOWEVENT_CLOSE: {
            // if we close the main window, close the whole frontend
            // obviously, this isn't ideal, but its good enough for me for now!
            if (auto id = SDL_GetWindowID(this->emu_instances[0].window); id == e.windowID) {
                this->running = false;
            } else { // otherwise, close the second window...
                if (this->emu_instances[1].HasWindow()) {
                    id = SDL_GetWindowID(this->emu_instances[1].window);
                    // make sure...
                    assert(id == e.windowID);
                    // close with window!
                    this->emu_instances[1].CloseRom(true);
                    // we won't be in dual mode anymore
                    this->run_state = EmuRunState::SINGLE;
                    // disconnect link cable also from gb0
                    GB_connect_link_cable(this->emu_instances[0].GetGB(), NULL, NULL);
                }
            }
        } break;
    }
}

#if SDL_VERSION_ATLEAST(2, 0, 9)
auto App::OnDisplayEvent(const SDL_DisplayEvent& e) -> void {
   switch (e.event) {
       case SDL_DISPLAYEVENT_NONE:
           break;

       case SDL_DISPLAYEVENT_ORIENTATION:
           break;
   }
}
#endif

auto App::OnKeyEvent(const SDL_KeyboardEvent& e) -> void {
    const GB_BOOL kdown = e.type == SDL_KEYDOWN;

    // first check if any of the mapped keys were pressed...
    for (auto [key, button] : key_map) {
        if (key == e.keysym.sym) {
            GB_set_buttons(this->emu_instances[0].GetGB(), button, kdown);
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
            this->FilePicker();
            break;

    // these are hotkeys to toggle layers of the gb core
    // such as bg, win, obj...
        case SDLK_0:
            GB_set_render_palette_layer_config(this->emu_instances[0].GetGB(), GB_RENDER_LAYER_CONFIG_ALL);
            break;

        case SDLK_1:
            GB_set_render_palette_layer_config(this->emu_instances[0].GetGB(), GB_RENDER_LAYER_CONFIG_BG);
            break;

        case SDLK_2:
            GB_set_render_palette_layer_config(this->emu_instances[0].GetGB(), GB_RENDER_LAYER_CONFIG_WIN);
            break;

        case SDLK_3:
            GB_set_render_palette_layer_config(this->emu_instances[0].GetGB(), GB_RENDER_LAYER_CONFIG_OBJ);
            break;
    }
}

auto App::OnJoypadAxisEvent(const SDL_JoyAxisEvent&) -> void {

}

auto App::OnJoypadButtonEvent(const SDL_JoyButtonEvent&) -> void {

}

auto App::OnJoypadHatEvent(const SDL_JoyHatEvent&) -> void {

}

auto App::OnJoypadDeviceEvent(const SDL_JoyDeviceEvent&) -> void {

}

auto App::OnControllerAxisEvent(const SDL_ControllerAxisEvent&) -> void {

}

auto App::OnControllerButtonEvent(const SDL_ControllerButtonEvent& e) -> void {
    assert(e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP);
    
    if (!this->HasController(e.which)) {
        log::log("unkown controller unkown cbutton" + std::to_string(e.button));
        return;
    }

    const bool down = e.type == SDL_CONTROLLERBUTTONDOWN;

    // the controller is used for the first window by default
    auto* gb = this->emu_instances[0].GetGB();

    // however, if 2 instances are open, the controller becomes the selected
    // input for p2!
    if (this->emu_instances[1].HasRom()) {
        gb = this->emu_instances[1].GetGB();
    }

    assert(gb != nullptr);

    switch (e.button) {
        case SDL_CONTROLLER_BUTTON_A: GB_set_buttons(gb, GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_Y: GB_set_buttons(gb, GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_B: GB_set_buttons(gb, GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_X: GB_set_buttons(gb, GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_BACK: GB_set_buttons(gb, GB_BUTTON_SELECT, down); break;
        case SDL_CONTROLLER_BUTTON_START: GB_set_buttons(gb, GB_BUTTON_START, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: GB_set_buttons(gb, GB_BUTTON_UP, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: GB_set_buttons(gb, GB_BUTTON_DOWN, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: GB_set_buttons(gb, GB_BUTTON_LEFT, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: GB_set_buttons(gb, GB_BUTTON_RIGHT, down); break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: break;
        case SDL_CONTROLLER_BUTTON_GUIDE: break; // home
        default:
            log::log("unkown cbutton %u\n", e.button);
            break;
    }
}

auto App::OnControllerDeviceEvent(const SDL_ControllerDeviceEvent& e) -> void {
    if (e.type == SDL_CONTROLLERDEVICEADDED) {
        log::log("CONTROLLER ADDED");
        this->AddController(e.which);
    } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
        log::log("CONTROLLER REMOVED");
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

auto App::OnTouchEvent(const SDL_TouchFingerEvent&) -> void {

}

auto App::OnUserEvent(SDL_UserEvent&) -> void {

}

} // namespace mgb
