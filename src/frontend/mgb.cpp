#include "frontend/mgb.hpp"

#include "frontend/util/io/romloader.hpp"
#include "frontend/util/io/ifile_cfile.hpp"
#include "frontend/util/io/ifile_gzip.hpp"
#include "frontend/util/util.hpp"
#include "frontend/util/log.hpp"

#include "frontend/platforms/video.hpp"
#include "frontend/platforms/audio.hpp"

#include "nativefiledialog/nfd.hpp"

#include "core/gb.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <map>


namespace mgb {


static void AudioCallback(struct GB_Core*, void* user_data, const struct GB_ApuCallbackData* data) {
    auto app = static_cast<App*>(user_data);
    app->OnAudioCallback(data);
}

auto App::OnAudioCallback(const struct GB_ApuCallbackData* data) -> void {
    this->audio_platform->PushSamples(data);
}

// vblank callback is to let the frontend know that it should render the screen.
// This removes any screen tearing and changes to the pixels whilst in vblank.
auto VblankCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnVblankCallback();
}

auto HblankCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnHblankCallback();
}

auto DmaCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnDmaCallback();
}

auto HaltCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnHaltCallback();
}

auto StopCallback(GB_Core*, void* user) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnStopCallback();
}

// error callback is to be used to handle errors that may happen during emulation.
// these will be updated with warnings also,
// such as trying to write to a rom or OOB read / write from the game.
auto ErrorCallback(GB_Core*, void* user, struct GB_ErrorData* data) -> void {
    assert(user);
    auto* app = static_cast<App*>(user);
    app->OnErrorCallback(data);
}

App::App() {
    const platform::video::Callbacks callbacks{
        /*.OnFileDrop =*/ [this](auto path){
            return this->LoadRom(path);
        },
        /*.OnAction =*/ [this](Action action, bool down){
            this->OnAction(action, down);
        },
        /*.OnQuit =*/ [this](){
            this->running = false;
        }
    };

    const platform::video::VideoInfo vid_info{
        /*.name =*/ "TotalGB",
        /*.render_type =*/ platform::video::RenderType::SOFTWARE,
        /*.x =*/ 0,
        /*.y =*/ 0,
        /*.w =*/ 160 * (int)this->options.GetScale(),
        /*.h =*/ 144 * (int)this->options.GetScale(),
    };

    const platform::video::GameTextureInfo game_info{
        /*.x =*/ 0,
        /*.y =*/ 0,
        /*.w =*/ 160,
        /*.h =*/ 144
    };

#ifdef MGB_SDL2_VIDEO
    using VideoPlatform = platform::video::sdl2::SDL2;
#elif MGB_SDL2GL_VIDEO
    using VideoPlatform = platform::video::sdl2::SDL2_GL
#elif MGB_ALLEGRO5_VIDEO
    using VideoPlatform = platform::video::allegro5::Allegro5;
#endif

#ifdef MGB_SDL2_AUDIO
    using AudioPlatform = platform::audio::sdl2::SDL2;
#elif MGB_SDL1_AUDIO
    using AudioPlatform = platform::audio::sdl1::SDL1;
#elif MGB_ALLEGRO5_AUDIO
    using AudioPlatform = platform::audio::allegro5::Allegro5;
#endif

    this->video_platform = std::make_unique<VideoPlatform>(callbacks);
    this->audio_platform = std::make_unique<AudioPlatform>();
    
    this->video_platform->SetupVideo(
        vid_info, game_info
    );

    this->audio_platform->SetupAudio();
}

App::~App() {
    this->CloseRom();
}

auto App::OnVblankCallback() -> void {
    // todo: send to video platform here!
    this->video_platform->UpdateGameTexture(
        platform::video::GameTextureData{
            /*.pixels =*/ (uint16_t*)gameboy->ppu.pixles,
            /*.w =*/ 160,
            /*.h =*/ 144
        }
    );
}

auto App::OnHblankCallback() -> void {

}

auto App::OnDmaCallback() -> void {

}

auto App::OnHaltCallback() -> void {

}

auto App::OnStopCallback() -> void {
    printf("[WARN] cpu stop instruction called!\n");
}

auto App::OnErrorCallback(struct GB_ErrorData* data) -> void {
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

auto App::SaveState() -> void {
    if (!this->HasRom()) {
        return;
    }

    auto state_path = util::getStatePathFromString(this->rom_path);

    io::Gzip file{state_path, "wb"};
    if (file.good()) {
        auto state = std::make_unique<struct GB_CoreState>();
        GB_savestate2(this->GetCore(), state.get());
        file.write((u8*)state.get(), sizeof(struct GB_CoreState));
    }
}

auto App::LoadState() -> void {
    if (!this->HasRom()) {
        return;
    }

    auto state_path = util::getStatePathFromString(this->rom_path);

    io::Gzip file{state_path, "rb"};
    if (file.good()) {
        auto state = std::make_unique<struct GB_CoreState>();
        file.read((u8*)state.get(), sizeof(struct GB_CoreState));
        GB_loadstate2(this->GetCore(), state.get());
    }
}

auto App::LoadRomInternal(const std::string& path) -> bool {
    // if we have a rom alread loaded, try and save the game
    // first before exiting...
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    } else {
        this->gameboy = std::make_unique<GB_Core>();
        GB_init(this->GetCore());

        GB_set_rtc_update_config(this->GetCore(), GB_RTC_UPDATE_CONFIG_USE_LOCAL_TIME);

        GB_set_apu_callback(this->GetCore(), AudioCallback, this);
        GB_set_vblank_callback(this->GetCore(), VblankCallback, this);
        GB_set_hblank_callback(this->GetCore(), HblankCallback, this);
        GB_set_dma_callback(this->GetCore(), DmaCallback, this);
        GB_set_halt_callback(this->GetCore(), HaltCallback, this);
        GB_set_stop_callback(this->GetCore(), StopCallback, this);
        GB_set_error_callback(this->GetCore(), ErrorCallback, this);
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

    if (-1 == GB_loadrom_data(
        this->gameboy.get(),
        this->rom_data.data(), this->rom_data.size())
    ) {
        printf("failed to load rom...\n");
        return false;
    }

    // save the path and set that the rom had loaded!
    this->rom_path = path;

    // try and set the rom name in window title
    {
        struct GB_CartName cart_name;
        if (!GB_get_rom_name(this->gameboy.get(), &cart_name)) {
            this->video_platform->SetWindowName(cart_name.name);
        }
    }

    // try and load a savefile (if any...)
    this->LoadSave(this->rom_path);

    return true;
}

auto App::CloseRom() -> bool {
    if (this->HasRom()) {
        this->SaveGame(this->rom_path);
    }

    this->rom_loaded = false;

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
        if (save_data.has_rtc == true) {
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
            save_data.has_rtc = true;
        }
    }

    GB_loadsave(this->gameboy.get(), &save_data);
}

auto App::HasRom() const -> bool {
    return this->rom_loaded;
}

auto App::GetCore() -> GB_Core* {
    return this->gameboy.get();
}

auto App::LoadRom(const std::string& path) -> bool {
    if (this->LoadRomInternal(path)) {
        this->rom_loaded = true;
        return true;
    }

    this->rom_loaded = false;
    return false;
}

auto App::FilePicker() -> void {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePath outPath;

    // prepare filters for the dialog
    const nfdfilteritem_t filterItem[] = {
        { "Valid", "gb,gbc,sgb,zip,gzip" },
        { "Roms", "gb,gbc,sgb" },
        { "Zip", "zip,gzip" },
    };

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

auto App::Loop() -> void {
    while (this->running) {
        this->Events();

        if (this->HasRom()) {
            GB_run_frame(this->GetCore());
        }

        // render the screen
        this->Draw();
    }
}

auto App::Events() -> void {
    this->video_platform->PollEvents();
}

auto App::Draw() -> void {
    this->video_platform->RenderDisplay();
}

auto App::OnGameAction(Action action, bool down) -> void {
    static std::unordered_map<Action, GB_Button> buttons{
        { Action::GAME_A, GB_BUTTON_A },
        { Action::GAME_B, GB_BUTTON_B },
        { Action::GAME_START, GB_BUTTON_START },
        { Action::GAME_SELECT, GB_BUTTON_SELECT },
        { Action::GAME_DOWN, GB_BUTTON_DOWN },
        { Action::GAME_UP, GB_BUTTON_UP },
        { Action::GAME_LEFT, GB_BUTTON_LEFT },
        { Action::GAME_RIGHT, GB_BUTTON_RIGHT },
    };

    // only handle inputs if we have a rom loaded!
    if (!this->HasRom()) {
        return;
    }

    // TODO: we should also only handle inputs depending on what
    // mode we are in, ie, if in UI menu or in-game!

    GB_set_buttons(
        this->GetCore(),
        buttons[action],
        down
    );
}

auto App::OnUIAction(Action action, bool down) -> void {
    // for now, only handle when keys are then released...
    if (down) {
        return;
    }

    enum class UI_Button {
        UP = (int)Action::UI_UP,
        DOWN = (int)Action::UI_DOWN,
        LEFT = (int)Action::UI_LEFT,
        RIGHT = (int)Action::UI_RIGHT,
        SELECT = (int)Action::UI_SELECT,
        BACK = (int)Action::UI_BACK,
    };

    switch (static_cast<UI_Button>(action)) {
        case UI_Button::UP:
            break;

        case UI_Button::DOWN:
            break;

        case UI_Button::LEFT:
            break;

        case UI_Button::RIGHT:
            break;

        case UI_Button::SELECT:
            break;

        case UI_Button::BACK:
            break;
    }
}

auto App::OnSCAction(Action action, bool down) -> void {
    // for now, only handle when keys are then released...
    if (down) {
        return;
    }

    enum class Shortcut {
        EXIT = (int)Action::SC_EXIT,
        FILE_PICKER = (int)Action::SC_FILE_PICKER,
        SAVESTATE = (int)Action::SC_SAVESTATE,
        LOADSTATE = (int)Action::SC_LOADSTATE,
        FULLSCREEN = (int)Action::SC_FULLSCREEN,
    };

    switch (static_cast<Shortcut>(action)) {
        case Shortcut::EXIT:
            break;

        case Shortcut::FILE_PICKER:
            this->FilePicker();
            break;

        case Shortcut::SAVESTATE:
            this->SaveState();
            break;

        case Shortcut::LOADSTATE:
            this->LoadState();
            break;
        
        case Shortcut::FULLSCREEN:
            this->video_platform->ToggleFullscreen();
            break;
    }
}

auto App::OnAction(Action action, bool down) -> void {
    switch (action) {
        case Action::GAME_A:
        case Action::GAME_B:
        case Action::GAME_UP:
        case Action::GAME_DOWN:
        case Action::GAME_LEFT:
        case Action::GAME_RIGHT:
        case Action::GAME_START:
        case Action::GAME_SELECT:
            this->OnGameAction(action, down);
            break;

        case Action::UI_UP:
        case Action::UI_DOWN:
        case Action::UI_LEFT:
        case Action::UI_RIGHT:
        case Action::UI_SELECT:
        case Action::UI_BACK:
            this->OnUIAction(action, down);
            break;

        case Action::SC_EXIT:
        case Action::SC_FILE_PICKER:
        case Action::SC_SAVESTATE:
        case Action::SC_LOADSTATE:
        case Action::SC_FULLSCREEN:
            this->OnSCAction(action, down);
            break;
    }
}

} // namespace mgb
