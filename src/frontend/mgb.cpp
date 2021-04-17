#include "mgb.hpp"
#include "SDL.h"
#include "core/gb.h"
#include "core/accessories/printer.h"
#include "io/romloader.hpp"
#include "io/ifile_cfile.hpp"
#include "io/ifile_gzip.hpp"
#include "util/util.hpp"
#include "util/log.hpp"

#include "nativefiledialog/nfd.hpp"


#include <cstdio>
#include <cstring>
#include <cassert>
#include <fstream>

namespace mgb {

// global
static SDL_AudioDeviceID AUDIO_DEVICE_ID = 0;


static void AudioCallback(struct GB_Core*, void* user_data, const struct GB_ApuCallbackData* data) {
    auto instance = static_cast<Instance*>(user_data);
    instance->OnAudioCallback(data);
}

auto Instance::OnAudioCallback(const struct GB_ApuCallbackData* data) -> void {
    // https://wiki.libsdl.org/SDL_GetQueuedAudioSize
    // todo: use sdl stream api instead, this delay was a temp hack
    // for now that magically "works".

    while (SDL_GetQueuedAudioSize(AUDIO_DEVICE_ID) > (1024 * 4)) {
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
    std::memcpy(this->buffered_pixels, gameboy->ppu.pixles, sizeof(gameboy->ppu.pixles));
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

auto Instance::SaveState() -> void {
    if (!this->HasRom()) {
        return;
    }

    auto state_path = util::getStatePathFromString(this->rom_path);

    io::Gzip file{state_path, "wb"};
    if (file.good()) {
        auto state = std::make_unique<struct GB_CoreState>();
        GB_savestate2(this->GetGB(), state.get());
        file.write((u8*)state.get(), sizeof(struct GB_CoreState));
    }
}

auto Instance::LoadState() -> void {
    if (!this->HasRom()) {
        return;
    }

    auto state_path = util::getStatePathFromString(this->rom_path);

    io::Gzip file{state_path, "rb"};
    if (file.good()) {
        auto state = std::make_unique<struct GB_CoreState>();
        file.read((u8*)state.get(), sizeof(struct GB_CoreState));
        GB_loadstate2(this->GetGB(), state.get());
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
        // GB_connect_printer(this->GetGB(), this->printer.get(), NULL, NULL);

    #ifdef GB_SDL_AUDIO_CALLBACK_STREAM
        GB_set_apu_callback(this->GetGB(), core_sdl_stream_callback, this);
    #elif GB_SDL_AUDIO_CALLBACK
        GB_set_apu_callback(this->GetGB(), core_audio_cb, this);
    #else
        GB_set_apu_callback(this->GetGB(), AudioCallback, this);
    #endif
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
        if (save_data.has_rtc == true) {
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
            save_data.has_rtc = true;
        }
    }

    GB_loadsave(this->gameboy.get(), &save_data);
}

auto Instance::CreateWindow() -> void {
    #ifndef SCALE
    #define SCALE 1
    #endif

    this->window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * SCALE, 144 * SCALE, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_SOFTWARE);
    this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetWindowMinimumSize(this->window, 160, 144);

    SDL_RendererInfo info;
    SDL_GetRendererInfo(this->renderer, &info);
    printf("\nRENDERER-INFO\n");
    printf("\tname: %s\n", info.name);
    printf("\tflags: 0x%X\n", info.flags);
    printf("\tnum textures: %u\n", info.num_texture_formats);
    for (uint32_t i = 0; i < info.num_texture_formats; ++i) {
        printf("\t\ttexture_format %u: 0x%X\n", i, info.texture_formats[i]);
    }

    printf("\nAUDIO_DEVICE_NAMES\n");
    for (int i = 0; i <  SDL_GetNumAudioDevices(0); i++) {
        printf("\tname: %s\n", SDL_GetAudioDeviceName(i, 0));
    }

    printf("\nAUDIO_DRIVER_NAMES\n");
    for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        const char* driver_name = SDL_GetAudioDriver(i);
        printf("\tname: %s\n", driver_name);
    }

    const SDL_AudioSpec wanted{
        /* .freq = */ 48000,
        /* .format = */ AUDIO_S8,
        /* .channels = */ 2,
        /* .silence = */ 0, // calculated
        /* .samples = */ 512, // 512 * 2 (because stereo)
        /* .padding = */ 0,
        /* .size = */ 0, // calculated
        /* .callback = */ NULL,
        /* .userdata = */ NULL
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

    SDL_setenv("SDL_AUDIODRIVER", "sndio", 1);

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromFile("res/mappings/gamecontrollerdb.txt");
}

App::~App() {
    this->instance.CloseRom(true);

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

auto App::LoadRom(const std::string& path) -> bool {
    if (this->instance.LoadRom(path)) {
        this->run_state = EmuRunState::SINGLE;
        return true;
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

auto App::Loop() -> void {
    while (this->running) {
        // handle sdl2 events
        this->Events();

        GB_run_frame(this->instance.GetGB());

        // render the screen
        this->Draw();
    }
}

auto Instance::Draw() -> void {
    if (this->renderer == NULL) {
        return;
    }

    void* pixles; int pitch;
    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    std::memcpy(pixles, this->buffered_pixels, sizeof(this->buffered_pixels));
    SDL_UnlockTexture(texture);

    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
    SDL_RenderPresent(this->renderer);
}

auto App::Draw() -> void {
    this->instance.Draw();
}

} // namespace mgb
