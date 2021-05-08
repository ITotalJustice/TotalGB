#ifndef _TYPES_H_
#define _TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


enum Action {
    // only used when the
    Action_GAME_A = 1,
    Action_GAME_B,
    Action_GAME_UP,
    Action_GAME_DOWN,
    Action_GAME_LEFT,
    Action_GAME_RIGHT,
    Action_GAME_START,
    Action_GAME_SELECT,

    // only used when the ui menu is shown
    Action_UI_UP,
    Action_UI_DOWN,
    Action_UI_LEFT,
    Action_UI_RIGHT,
    Action_UI_SELECT,
    Action_UI_BACK,

    // shortcut
    Action_SC_EXIT,
    Action_SC_FILE_PICKER,
    Action_SC_SAVESTATE,
    Action_SC_LOADSTATE,
    Action_SC_FULLSCREEN,
};

#ifdef __cplusplus
}
#endif

#endif // _TYPES_H
