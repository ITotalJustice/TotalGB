#include "mgb.hpp"
#include "../core/gb.h"
#include "../core/accessories/printer.h"
#include "io/romloader.hpp"
#include "io/ifile_cfile.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

#include "nativefiledialog/nfd.hpp"

#include <SDL2/SDL_audio.h>

#include <cstring>
#include <cassert>
#include <fstream>

namespace mgb {

// global
static SDL_AudioDeviceID AUDIO_DEVICE_ID = 0;


static void AudioCallback(struct GB_Core*, void* user_data, struct GB_ApuCallbackData* data) {
    auto instance = static_cast<Instance*>(user_data);
    instance->OnAudioCallback(data);
}

auto Instance::OnAudioCallback(struct GB_ApuCallbackData* data) -> void {
    // https://wiki.libsdl.org/SDL_GetQueuedAudioSize
    // todo: use sdl stream api instead, this delay was a temp hack
    // for now that magically "works".
    while ((SDL_GetQueuedAudioSize(AUDIO_DEVICE_ID)) > (1024 << 2)) {
        SDL_Delay(1);
    }

    SDL_QueueAudio(AUDIO_DEVICE_ID, data->samples, sizeof(data->samples));
}
// vblank callback is to let the frontend know that it should render the screen.
// This removes any screen tearing and changes to the pixels whilst in vblank.
auto VblankCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnVblankCallback();
}

auto HblankCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHblankCallback();
}

auto DmaCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnDmaCallback();
}

auto HaltCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnHaltCallback();
}

auto StopCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* instance = static_cast<Instance*>(user);
    instance->OnStopCallback();
}

// error callback is to be used to handle errors that may happen during emulation.
// these will be updated with warnings also,
// such as trying to write to a rom or OOB read / write from the game.
auto ErrorCallback(GB_Core*, void* user, struct GB_ErrorData* data) -> void {
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
    printf("[WARN] cpu stop instruction called!\n");
}

auto Instance::OnErrorCallback(struct GB_ErrorData* data) -> void {
    switch (data->type) {
        case GB_ErrorType::GB_ERROR_TYPE_UNKNOWN_INSTRUCTION:
            printf("[ERROR] UNK instruction OP 0x%02X CB: %s\n", data->unk_instruction.opcode, data->unk_instruction.cb_prefix ? "TRUE" : "FALSE");
            break;

        case GB_ErrorType::GB_ERROR_TYPE_INFO:
            printf("[INFO] %s\n", data->info.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_WARN:
            printf("[WARN] %s\n", data->warn.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_ERROR:
            printf("[ERROR] %s\n", data->error.message);
            break;

        case GB_ErrorType::GB_ERROR_TYPE_UNK:
            printf("[ERROR] Unknown gb error...\n");
            break;
    }
}

auto Instance::LoadRom(const std::string& path) -> bool {
    // if we have a rom alread loaded, try and save the game
    // first before exiting...
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    } else {
        this->gameboy = std::make_unique<GB_Core>();
        this->printer = std::make_unique<GB_Printer>();
        GB_init(this->GetGB());

        GB_set_rtc_update_config(this->GetGB(), GB_RTC_UPDATE_CONFIG_USE_LOCAL_TIME);
        GB_connect_printer(this->GetGB(), this->printer.get(), NULL, NULL);

        GB_set_apu_callback(this->GetGB(), AudioCallback, this);
        GB_set_vblank_callback(this->GetGB(), VblankCallback, this);
        GB_set_hblank_callback(this->GetGB(), HblankCallback, this);
        GB_set_dma_callback(this->GetGB(), DmaCallback, this);
        GB_set_halt_callback(this->GetGB(), HaltCallback, this);
        GB_set_stop_callback(this->GetGB(), StopCallback, this);
        GB_set_error_callback(this->GetGB(), ErrorCallback, this);
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

    if (!this->HasWindow()) {
        this->CreateWindow();
    }

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

auto Instance::CloseRom(bool close_window) -> bool {
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    }

    if (close_window && this->HasWindow()) {
        this->gameboy.reset();
        this->DestroyWindow();
    }

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

auto Instance::CreateWindow() -> void {
    this->window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetWindowMinimumSize(this->window, 160, 144);
}

auto Instance::DestroyWindow() -> void {
    SDL_DestroyTexture(this->texture);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    this->texture = nullptr;
    this->renderer = nullptr;
    this->window = nullptr;
}

auto Instance::HasWindow() const -> bool {
    return this->window != nullptr;
}

auto Instance::HasRom() const -> bool {
    return this->gameboy != nullptr;
}

auto Instance::GetGB() -> GB_Core* {
    return this->gameboy.get();
}

#ifdef GB_SDL_AUDIO_CALLBACK
static void AudioCallback(void* user, u8* buf, int len) {
    auto instance = static_cast<Instance*>(user);
    memset(buf, 0, len);
    GB_SDL_audio_callback(instance->GetGB(), (s8*)buf, len);
}
#endif // GB_SDL_AUDIO_CALLBACK

App::App() {
    {
        SDL_version compiled;
        SDL_version linked;

        SDL_VERSION(&compiled);
        SDL_GetVersion(&linked);
        printf("We compiled against SDL version %d.%d.%d ...\n",
            compiled.major, compiled.minor, compiled.patch);
        printf("But we are linking against SDL version %d.%d.%d.\n",
            linked.major, linked.minor, linked.patch);
    }

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromFile("res/mappings/gamecontrollerdb.txt");

    const SDL_AudioSpec wanted{
        /* .freq = */ 48'000,
        /* .format = */ AUDIO_S8,
        /* .channels = */ 2,
        /* .silence = */ 0, // calculated
        /* .samples = */ 1024, // 512 * 2 (because stereo)
        /* .padding = */ 0,
        /* .size = */ 0, // calculated
#ifndef GB_SDL_AUDIO_CALLBACK
        /* .callback = */ NULL,
        /* .userdata = */ NULL
#else
        /* .callback = */ AudioCallback,
        /* .userdata = */ &this->emu_instances[0]
#endif // GB_SDL_AUDIO_CALLBACK
    };
    SDL_AudioSpec obtained{};

    AUDIO_DEVICE_ID = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    // check if an audio device was failed to be found...
    if (AUDIO_DEVICE_ID == 0) {
        printf("failed to find valid audio device\n");
    } else {
        printf("\nSDL_AudioSpec:\n");
        printf("\tfreq: %d\n", obtained.freq);
        printf("\tformat: %d\n", obtained.format);
        printf("\tchannels: %u\n", obtained.channels);
        printf("\tsilence: %u\n", obtained.silence);
        printf("\tsamples: %u\n", obtained.samples);
        printf("\tpadding: %u\n", obtained.padding);
        printf("\tsize: %u\n", obtained.size);

        SDL_PauseAudioDevice(AUDIO_DEVICE_ID, 0);
    }
}

App::~App() {
    for (auto& p : this->emu_instances) {
        // we want to close the window as well this time...
        p.CloseRom(true);
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

    // close audio device if opened...
    if (AUDIO_DEVICE_ID != 0) {
        SDL_CloseAudioDevice(AUDIO_DEVICE_ID);
        AUDIO_DEVICE_ID = 0;
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
            GB_connect_link_cable_builtin(this->emu_instances[0].GetGB(), this->emu_instances[1].GetGB());
            GB_connect_link_cable_builtin(this->emu_instances[1].GetGB(), this->emu_instances[0].GetGB());
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

void GB_run_frames(struct GB_Core* gb1, struct GB_Core* gb2) {
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
                GB_run_frame(this->emu_instances[0].GetGB());
                // GB_run_frame(this->emu_instances[0].GetGB());
                // GB_run_frame(this->emu_instances[0].GetGB());
                break;

            case EmuRunState::DUAL:
                GB_run_frame(this->emu_instances[0].GetGB());
                GB_run_frame(this->emu_instances[1].GetGB());
                // GB_run_frames(
                //     this->emu_instances[0].GetGB(),
                //     this->emu_instances[1].GetGB()
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
