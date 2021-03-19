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

App::App() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromFile("res/mappings/gamecontrollerdb.txt");

    // window
	this->window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	SDL_SetWindowMinimumSize(window, 160, 144);
    SDL_ShowCursor(SDL_DISABLE);

    this->gameboy = std::make_unique<GB_Data>();
    GB_init(this->gameboy.get());
}

App::~App() {
    // try and save the game before exiting...
    if (this->rom_loaded == true) {
        this->SaveGame(this->rom_path);
    }

	GB_quit(this->gameboy.get());

    // cleanup joysticks and controllers
    for (auto &p : this->joysticks) {
        SDL_JoystickClose(p.ptr);
        p.ptr = nullptr;
    }
    for (auto &p : this->controllers) {
        SDL_GameControllerClose(p.ptr);
        p.ptr = nullptr;
    }

    SDL_DestroyTexture(this->texture);
	SDL_DestroyRenderer(this->renderer);
	SDL_DestroyWindow(this->window);
	SDL_Quit();
}

auto App::LoadRom(const std::string& path) -> bool {
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

    if (-1 == GB_loadrom_data(this->gameboy.get(), this->rom_data.data(), this->rom_data.size(), NULL)) {
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

auto App::SaveGame(const std::string& path) -> void {
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

auto App::LoadSave(const std::string& path) -> void {
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

auto App::Loop() -> void {
    while (this->running) {
        this->Events();
        GB_run_frame(gameboy.get());
        this->Draw();
    }
}

auto App::Draw() -> void {
    SDL_RenderClear(renderer);
    void* pixles; int pitch;

    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    memcpy(pixles, gameboy->ppu.pixles, sizeof(gameboy->ppu.pixles));
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

} // namespace mgb
