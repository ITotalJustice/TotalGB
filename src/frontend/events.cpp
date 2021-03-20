#include "mgb.hpp"
#include "../core/gb.h"
#include "util/util.hpp"
#include "util/log.hpp"

#include <cassert>

namespace mgb {

struct KeyMapEntry {
    int key;
    enum GB_Button button;
};

constexpr KeyMapEntry key_map[]{
    {SDLK_x, GB_BUTTON_A},
    {SDLK_z, GB_BUTTON_B},
    {SDLK_RETURN, GB_BUTTON_START},
    {SDLK_SPACE, GB_BUTTON_SELECT},
    {SDLK_DOWN, GB_BUTTON_DOWN},
    {SDLK_UP, GB_BUTTON_UP},
    {SDLK_LEFT, GB_BUTTON_LEFT},
    {SDLK_RIGHT, GB_BUTTON_RIGHT},
};

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

            case SDL_DISPLAYEVENT:
                this->OnDisplayEvent(e.display);
                break;

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

        case SDL_WINDOWEVENT_CLOSE:
            break;
    }
}

auto App::OnDisplayEvent(const SDL_DisplayEvent& e) -> void {
    switch (e.event) {
        case SDL_DISPLAYEVENT_NONE:
            break;

        case SDL_DISPLAYEVENT_ORIENTATION:
            break;
    }
}

auto App::OnKeyEvent(const SDL_KeyboardEvent& e) -> void {
    const GB_BOOL kdown = e.type == SDL_KEYDOWN;

    // first check if any of the mapped keys were pressed...
    for (size_t i = 0; i < GB_ARR_SIZE(key_map); ++i) {
        if (key_map[i].key == e.keysym.sym) {
            GB_set_buttons(this->emu_instances[0].gameboy.get(), key_map[i].button, kdown);
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

    switch (e.button) {
        case SDL_CONTROLLER_BUTTON_A: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_Y: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_B, down); break;
        case SDL_CONTROLLER_BUTTON_B: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_X: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_A, down); break;
        case SDL_CONTROLLER_BUTTON_BACK: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_SELECT, down); break;
        case SDL_CONTROLLER_BUTTON_START: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_START, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_UP, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_DOWN, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_LEFT, down); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: GB_set_buttons(this->emu_instances[1].gameboy.get(), GB_BUTTON_RIGHT, down); break;
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
