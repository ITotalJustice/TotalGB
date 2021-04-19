#pragma once

#include <cstdint>
#include <cstddef>


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


enum class Action {
    // only used when the 
    GAME_A,
    GAME_B,
    GAME_UP,
    GAME_DOWN,
    GAME_LEFT,
    GAME_RIGHT,
    GAME_START,
    GAME_SELECT,

    // only used when the ui menu is shown
    UI_UP,
    UI_DOWN,
    UI_LEFT,
    UI_RIGHT,
    UI_SELECT,
    UI_BACK,

    // shortcut
    SC_EXIT,
    SC_FILE_PICKER,
    SC_SAVESTATE,
    SC_LOADSTATE,
    SC_FULLSCREEN,
};

} // namespace mgb
