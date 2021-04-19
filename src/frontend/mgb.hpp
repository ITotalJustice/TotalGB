#include <cstdint>
#include <array>
#include <memory>
#include <vector>
#include <string>


#include "frontend/platforms/video/interface.hpp"
#include "frontend/platforms/audio/interface.hpp"


extern "C" {
struct GB_Core;
struct GB_ErrorData;
struct GB_Printer;
struct GB_ApuCallbackData;
}


namespace mgb {


using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;


enum class EmuRunState {
    NONE,
    SINGLE,
};


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
    size_t scale = 1;
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

    auto GetGB() -> GB_Core*;

private:
    std::unique_ptr<GB_Core> gameboy;

    std::unique_ptr<platform::video::Interface> video_platform;
    std::unique_ptr<platform::audio::Interface> audio_platform;

    std::vector<u8> rom_data;
    std::string rom_path;

    Options options;
    
    EmuRunState run_state{EmuRunState::NONE};

    bool running{true};
};

} // namespace mgb
