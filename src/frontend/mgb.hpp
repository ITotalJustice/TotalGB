#include "frontend/types.hpp"
#include "frontend/platforms/video/interface.hpp"
#include "frontend/platforms/audio/interface.hpp"

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <functional>


namespace mgb {


class Options final {
public:
    auto SetScale(size_t new_scale) {
        // cannot be zero...
        this->scale = new_scale + (new_scale == 0);
    }

    auto GetScale() const {
        return this->scale;
    }

private:
    size_t scale = 3;
};

class App final {
public:
// some platforms need to manually sync the files
// ie, for web port, sync must be perfored after writes
// some syncs for platforms will be async (like web), so
// a callback might be a good idea, to maybe block
// before another write, or to let the user know that a write
// is in progress and when it has completed.
// this is VERY important for saves, as losing save data should NEVER happen!
using OnSaveWriteCB = std::function<void()>;
using OnStateWriteCB = std::function<void()>;

public:
    App();
    ~App();

    auto LoadRom(const std::string& path) -> bool;

    // the path is the file path that was used to load the data.
    // if there is no path (ie, the rom is builtin), then a path should
    // still be set, as this is used for loading saves and states.
    // an example for loading tetris would be setting the path to
    // "pokemon_red.gb", so saves will be "pokemon_red.sav" etc.
    auto LoadRomData(const std::string& path, const std::uint8_t* data, std::size_t size) -> bool;
    
    auto Loop() -> void;
    auto LoopStep() -> void;

    // writes out the save to file.
    // this is normally done on exit, or when a new rom is loaded.
    // however, on some platforms, it may be desirable to flush
    // periodically, such as [web] (atexit isn't called).
    auto FlushSave() -> void;

    // tby default, saves and states are loaded / saved from the same
    // folder that the rom was loaded from.
    // however, by setting the path, the folder.
    auto SetSavePath(std::string path) { this->custom_save_path = path; }
    auto SetRtcPath(std::string path) { this->custom_rtc_path = path; }
    auto SetStatePath(std::string path) { this->custom_state_path = path; }

    auto SetWriteSaveCB(OnSaveWriteCB cb) { this->on_save_write_cb = cb; }
    auto SetWriteStateCB(OnStateWriteCB cb) { this->on_state_write_cb = cb; }

public:
    auto OnAudioCallback(const struct GB_ApuCallbackData* data) -> void;
    auto OnVblankCallback() -> void;
    auto OnHblankCallback() -> void;
    auto OnDmaCallback() -> void;
    auto OnHaltCallback() -> void;
    auto OnStopCallback() -> void;
    auto OnErrorCallback(struct GB_ErrorData* data) -> void;

private:
    struct LoadRomInfo {
        enum class Type { FILE, MEM };
        
        LoadRomInfo(std::string _path)
            : path{_path}, type{Type::FILE} {}

        LoadRomInfo(std::string _path, const std::uint8_t* _data, std::size_t _size)
            : path{_path}, data{_data}, size{_size}, type{Type::MEM} {}
            
        std::string path;
        const std::uint8_t* data{};
        std::size_t size{};
        Type type;
    };

private:
    auto LogToDisplay(const std::string& text) -> void;
    
    auto LoadRomInternal(LoadRomInfo&& info) -> bool;
    auto FilePicker() -> void;

    auto Draw() -> void;
    auto Events() -> void;

    auto HasRom() const -> bool;

    auto CloseRom() -> bool;

    auto GetSavePath() -> std::string;
    auto GetRtcPath() -> std::string;
    auto GetStatePath() -> std::string;

    auto SaveGame() -> void;
    auto LoadSave() -> void;

    auto SaveState() -> void;
    auto LoadState() -> void;

    auto GetCore() -> GB_Core*;

    auto OnGameAction(Action action, bool down) -> void;
    auto OnUIAction(Action action, bool down) -> void;
    auto OnSCAction(Action action, bool down) -> void;

    auto OnAction(Action key, bool down) -> void;

private:
    std::unique_ptr<GB_Core> gameboy;

    std::unique_ptr<platform::video::Interface> video_platform;
    std::unique_ptr<platform::audio::Interface> audio_platform;

    std::vector<u8> rom_data;
    std::string rom_path;

    Options options;

    // todo: check if these init to empty!
    std::string custom_save_path;
    std::string custom_rtc_path;
    std::string custom_state_path;

    OnSaveWriteCB on_save_write_cb{};
    OnStateWriteCB on_state_write_cb{};

    bool rom_loaded{false};
    bool running{true};
};

} // namespace mgb
