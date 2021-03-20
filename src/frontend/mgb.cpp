#include "mgb.hpp"
#include "../core/gb.h"
#include "io/romloader.hpp"
#include "io/ifile_cfile.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

#include "nativefiledialog/nfd.hpp"

#include <cstring>
#include <cassert>
#include <fstream>

namespace mgb {

// vblank callback is to let the frontend know that it should render the screen.
// This removes any screen tearing and changes to the pixels whilst in vblank.
auto VblankCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnVblankCallback();
}

auto HblankCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHblankCallback();
}

auto DmaCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnDmaCallback();
}

auto HaltCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHaltCallback();
}

auto StopCallback(GB_Data*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnStopCallback();
}

// error callback is to be used to handle errors that may happen during emulation.
// these will be updated with warnings also,
// such as trying to write to a rom or OOB read / write from the game.
auto ErrorCallback(GB_Data*, void* user, struct GB_ErrorData* data) -> void {
    assert(user);
    auto* app = static_cast<Instance*>(user);
    app->OnErrorCallback(data);
}

auto Instance::OnVblankCallback() -> void {
    void* pixles; int pitch;

    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    std::memcpy(pixles, gameboy->ppu.pixles, sizeof(gameboy->ppu.pixles));
    SDL_UnlockTexture(texture);
}

auto Instance::OnHblankCallback() -> void {

}

auto Instance::OnDmaCallback() -> void {

}

auto Instance::OnHaltCallback() -> void {

}

auto Instance::OnStopCallback() -> void {
    printf("[ERROR] cpu stop instruction called!\n");
}

auto Instance::OnErrorCallback(struct GB_ErrorData* data) -> void {
    switch (data->code) {
        case GB_ErrorCode::GB_ERROR_CODE_UNKNOWN_INSTRUCTION:
            printf("[ERROR] UNK instruction OP 0x%02X CB: %s\n", data->unk_instruction.opcode, data->unk_instruction.cb_prefix ? "TRUE" : "FALSE");
            break;

        case GB_ErrorCode::GB_ERROR_CODE_UNK:
            printf("[ERROR] Unknown gb error...\n");
            break;
    }
}

auto Instance::LoadRom(const std::string& path) -> bool {
    // if we have a rom alread loaded, try and save the game
    // first before exiting...
    if (this->rom_loaded == true) {
        this->SaveGame(this->rom_path);
    }

    io::RomLoader romloader{path};
    if (!romloader.good()) {
        return false;
    }

    // get the size
    const auto file_size = romloader.size();


    // resize vector
    this->rom_data.resize(file_size);

    // read entire file...
    romloader.read(this->rom_data.data(), this->rom_data.size());

    if (-1 == GB_loadrom_data(this->gameboy.get(), this->rom_data.data(), this->rom_data.size())) {
        return false;
    }

    // save the path and set that the rom had loaded!
    this->rom_path = path;
    this->rom_loaded = true;

    // try and set the rom name in window title
    {
        struct GB_CartName cart_name;
        if (0 == GB_get_rom_name(this->gameboy.get(), &cart_name)) {
            SDL_SetWindowTitle(this->window, cart_name.name);
        }
    }

    // try and load a savefile (if any...)
    this->LoadSave(this->rom_path);

    return true;
}

auto Instance::SaveGame(const std::string& path) -> void {
    if (GB_has_save(this->gameboy.get()) || GB_has_rtc(this->gameboy.get())) {
        struct GB_SaveData save_data;
        GB_savegame(this->gameboy.get(), &save_data);
        
        // save sram
        if (save_data.size > 0) {
            const auto save_path = util::getSavePathFromString(path);
            io::Cfile file{save_path, "wb"};
            if (file.good()) {
                file.write(save_data.data, save_data.size);
            }
        }

        // save rtc
        if (save_data.has_rtc == GB_TRUE) {
            const auto save_path = util::getRtcPathFromString(path);
            io::Cfile file{save_path, "wb"};
            if (file.good()) {
                file.write((u8*)&save_data.rtc, sizeof(save_data.rtc));
            }
        }
    }
}

auto Instance::LoadSave(const std::string& path) -> void {
    struct GB_SaveData save_data{};

    // load sram
    if (GB_has_save(this->gameboy.get())) {
        printf("has save!\n");

        const auto save_path = util::getSavePathFromString(path);
        const auto save_size = GB_calculate_savedata_size(this->gameboy.get());
        
        io::Cfile file{save_path, "rb"};

        if (file.good() && file.size() == save_size) {
            printf("trying to read... %u\n", save_size);
            file.read(save_data.data, save_size);
            save_data.size = save_size;
        }
    }
    
    // load rtc
    if (GB_has_rtc(this->gameboy.get())) {
        const auto save_path = util::getRtcPathFromString(path);
        io::Cfile file{save_path, "rb"};

        if (file.good() && file.size() == sizeof(save_data.rtc)) {
            file.read((u8*)&save_data.rtc, sizeof(save_data.rtc));
            save_data.has_rtc = GB_TRUE;
        }
    }

    GB_loadsave(this->gameboy.get(), &save_data);
}

App::App() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromFile("res/mappings/gamecontrollerdb.txt");

    // SDL_ShowCursor(SDL_DISABLE);

    for (std::size_t i = 0; i < this->emu_instances.size(); ++i) {
        this->emu_instances[i].window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, SDL_WINDOW_SHOWN);
        this->emu_instances[i].renderer = SDL_CreateRenderer(this->emu_instances[i].window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	    this->emu_instances[i].texture = SDL_CreateTexture(this->emu_instances[i].renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
        SDL_SetWindowMinimumSize(this->emu_instances[i].window, 160, 144);

        this->emu_instances[i].gameboy = std::make_unique<GB_Data>();
        GB_init(this->emu_instances[i].gameboy.get());

        GB_set_vblank_callback(this->emu_instances[i].gameboy.get(), VblankCallback, &this->emu_instances[i]);
        GB_set_hblank_callback(this->emu_instances[i].gameboy.get(), HblankCallback, &this->emu_instances[i]);
        GB_set_dma_callback(this->emu_instances[i].gameboy.get(), DmaCallback, &this->emu_instances[i]);
        GB_set_halt_callback(this->emu_instances[i].gameboy.get(), HaltCallback, &this->emu_instances[i]);
        GB_set_stop_callback(this->emu_instances[i].gameboy.get(), StopCallback, &this->emu_instances[i]);
        GB_set_error_callback(this->emu_instances[i].gameboy.get(), ErrorCallback, &this->emu_instances[i]);
    }
}

App::~App() {
    for (auto& p : this->emu_instances) {
        if (p.rom_loaded) {
            p.SaveGame(p.rom_path);
        }

        GB_quit(p.gameboy.get());

        // destroy first instance
        SDL_DestroyTexture(p.texture);
        SDL_DestroyRenderer(p.renderer);
        SDL_DestroyWindow(p.window);
    }

    // cleanup joysticks and controllers
    for (auto &p : this->joysticks) {
        SDL_JoystickClose(p.ptr);
        p.ptr = nullptr;
    }

    for (auto &p : this->controllers) {
        SDL_GameControllerClose(p.ptr);
        p.ptr = nullptr;
    }

	SDL_Quit();
}

auto App::LoadRom(const std::string& path, bool dual) -> bool {
    // load the game in slot 1
    if (dual == false) {
        if (this->emu_instances[0].LoadRom(path)) {
            this->run_state = EmuRunState::SINGLE;
            return true;
        }
    } else { // else slot 2
        if (this->emu_instances[1].LoadRom(path)) {
            this->run_state = EmuRunState::DUAL;
            // connect both games via link cable
            GB_connect_link_cable_builtin(this->emu_instances[0].gameboy.get(), this->emu_instances[1].gameboy.get());
            GB_connect_link_cable_builtin(this->emu_instances[1].gameboy.get(), this->emu_instances[0].gameboy.get());
            return true;
        }
    }

    this->run_state = EmuRunState::NONE;
    return false;
}

auto App::FilePicker() -> void {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePath outPath;

    // prepare filters for the dialog
    const nfdfilteritem_t filterItem[2] = { { "Roms", "gb,gbc" },{ "Zip", "zip,gzip" } };

    // show the dialog
    const auto result = NFD::OpenDialog(outPath, filterItem, sizeof(filterItem) / sizeof(filterItem[0]));

    switch (result) {
        case NFD_ERROR:
            printf("[ERROR] failed to open file...\n");
            break;

        case NFD_CANCEL:
            printf("[INFO] cancled open file request...\n");
            break;

        case NFD_OKAY:
            this->LoadRom(outPath.get());
            break;
    }
}

auto App::HasController(int which) const -> bool {
    for (auto &p : this->controllers) {
        if (p.id == which) {
            return true;
        }
    }
    return false;
}

auto App::AddController(int index) -> bool {
    auto controller = SDL_GameControllerOpen(index);
    if (!controller) {
        return log::errReturn(false, "Failed to open controller from index %d\n", index);
    }

    ControllerCtx ctx;
    ctx.ptr = controller;
    ctx.id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
    this->controllers.push_back(std::move(ctx));

    return true;
}

auto App::ResizeScreen() -> void {

}

void GB_run_frames(struct GB_Data* gb1, struct GB_Data* gb2) {
	GB_U32 cycles_elapsed1 = 0;
    GB_U32 cycles_elapsed2 = 0;

    while (cycles_elapsed1 < GB_FRAME_CPU_CYCLES || cycles_elapsed2 < GB_FRAME_CPU_CYCLES) {
        if (cycles_elapsed1 < GB_FRAME_CPU_CYCLES) {
            cycles_elapsed1 += GB_run_step(gb1);
        }
        if (cycles_elapsed2 < GB_FRAME_CPU_CYCLES) {
            cycles_elapsed2 += GB_run_step(gb2);
        }
    }
}

auto App::Loop() -> void {
    while (this->running) {
        // handle sdl2 events
        this->Events();

        switch (this->run_state) {
            case EmuRunState::NONE:
                break;

            case EmuRunState::SINGLE:
                GB_run_frame(this->emu_instances[0].gameboy.get());
                break;

            case EmuRunState::DUAL:
                GB_run_frame(this->emu_instances[0].gameboy.get());
                GB_run_frame(this->emu_instances[1].gameboy.get());
                // GB_run_frames(
                //     this->emu_instances[0].gameboy.get(),
                //     this->emu_instances[1].gameboy.get()
                // );
                break;
        }

        // render the screen
        this->Draw();
    }
}

auto App::Draw() -> void {
    for (auto& p : this->emu_instances) {
        SDL_RenderClear(p.renderer);
        SDL_RenderCopy(p.renderer, p.texture, NULL, NULL);
        SDL_RenderPresent(p.renderer);
    }
}

} // namespace mgb
