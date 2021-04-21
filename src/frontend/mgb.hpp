#include "frontend/types.hpp"
#include "frontend/platforms/video/interface.hpp"
#include "frontend/platforms/audio/interface.hpp"

#include <array>
#include <memory>
#include <vector>
#include <string>


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
    size_t scale = 4;
};

class App final {
public:
    App();
    ~App();

    auto LoadRom(const std::string& path) -> bool;
    auto Loop() -> void;

public:
    auto OnAudioCallback(const struct GB_ApuCallbackData* data) -> void;
    auto OnVblankCallback() -> void;
    auto OnHblankCallback() -> void;
    auto OnDmaCallback() -> void;
    auto OnHaltCallback() -> void;
    auto OnStopCallback() -> void;
    auto OnErrorCallback(struct GB_ErrorData* data) -> void;

private:
    auto LoadRomInternal(const std::string& path) -> bool;
    auto FilePicker() -> void;

    auto Draw() -> void;
    auto Events() -> void;

    auto HasRom() const -> bool;

    auto CloseRom() -> bool;

    auto SaveGame(const std::string& path) -> void;
    auto LoadSave(const std::string& path) -> void;

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

    bool rom_loaded{false};
    bool running{true};
};

} // namespace mgb
