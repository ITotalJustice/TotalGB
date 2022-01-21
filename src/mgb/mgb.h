#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef HAS_SDL2
    #include "sdl_helper.h"
#endif

enum CallbackType
{
    CallbackType_LOAD_ROM,
    CallbackType_LOAD_SAVE,
    CallbackType_LOAD_STATE,

    CallbackType_SAVE_SAVE,
    CallbackType_SAVE_STATE,
};

struct GB_Core;

bool mgb_init(struct GB_Core* gb);
void mgb_exit(void);

void mgb_set_on_file_callback(void (*cb)(const char*, enum CallbackType, bool));

void mgb_set_save_folder(const char* path);
void mgb_set_rtc_folder(const char* path);
void mgb_set_state_folder(const char* path);

const char* mgb_get_save_folder(void);
const char* mgb_get_rtc_folder(void);
const char* mgb_get_state_folder(void);

bool mgb_load_rom_filedialog(void);
bool mgb_load_rom_file(const char* path);
bool mgb_load_rom_fd(int fd, bool own, const char* path);
bool mgb_load_rom_data(const char* path, const uint8_t* data, size_t size);

// setting the path=NULL will use the current rom_path
// to create the new path, eg, if rom_name = rom.bin
// then the output for a save will be rom.sav.
bool mgb_load_save_file(const char* path);
bool mgb_load_rtc_file(const char* path);
bool mgb_load_state_file(const char* path);

bool mgb_load_save_data(const uint8_t* data, size_t size);
bool mgb_load_rtc_data(const uint8_t* data, size_t size);
bool mgb_load_state_data(const uint8_t* data, size_t size);

bool mgb_load_state_filedialog(void);
bool mgb_save_state_filedialog(void);

bool mgb_save_save_file(const char* path);
bool mgb_save_rtc_file(const char* path);
bool mgb_save_state_file(const char* path);

// return true if rom is loaded
bool mgb_has_rom(void);

bool mgb_rewind_init(size_t seconds);
void mgb_rewind_close(void);

// save states and stores pixel data for that frame
bool mgb_rewind_push_frame(const void* pixels, size_t size);
// loads state and loads pixel data for that frame
bool mgb_rewind_pop_frame(void* pixels, size_t size);

// // event
// enum MgbEventMouseButton {
//     MgbEventMouseButton_NONE = 0,

//     MgbEventMouseButton_LEFT,
//     MgbEventMouseButton_MIDDLE,
//     MgbEventMouseButton_RIGHT,

//     MgbEventMouseButton_MAX,
// };

// enum MgbEventControllerButton {
//     MgbEventControllerButton_NONE = 0,

//     MgbEventControllerButton_A,
//     MgbEventControllerButton_B,
//     MgbEventControllerButton_X,
//     MgbEventControllerButton_Y,

//     MgbEventControllerButton_START,
//     MgbEventControllerButton_SELECT,
//     MgbEventControllerButton_HOME,

//     MgbEventControllerButton_L1,
//     MgbEventControllerButton_L3,
//     MgbEventControllerButton_R1,
//     MgbEventControllerButton_R3,

//     MgbEventControllerButton_UP,
//     MgbEventControllerButton_DOWN,
//     MgbEventControllerButton_LEFT,
//     MgbEventControllerButton_RIGHT,

//     MgbEventControllerButton_MAX
// };

// enum MgbEventControllerAxis {
//     MgbEventControllerAxis_NONE = 0,

//     MgbEventControllerAxis_L2,
//     MgbEventControllerAxis_R2,
//     MgbEventControllerAxis_LEFTX,
//     MgbEventControllerAxis_LEFTY,
//     MgbEventControllerAxis_RIGHTX,
//     MgbEventControllerAxis_RIGHTY,

//     MgbEventControllerAxis_MAX,
// };

// // not yes finished!
// enum MgbEventKey {
//     MgbEventKey_NONE = 0,

//     MgbEventKey_Q,
//     MgbEventKey_W,
//     MgbEventKey_E,
//     MgbEventKey_R,
//     MgbEventKey_T,
//     MgbEventKey_Y,
//     MgbEventKey_U,
//     MgbEventKey_I,
//     MgbEventKey_O,
//     MgbEventKey_P,
//     MgbEventKey_A,
//     MgbEventKey_S,
//     MgbEventKey_D,
//     MgbEventKey_F,
//     MgbEventKey_G,
//     MgbEventKey_H,
//     MgbEventKey_J,
//     MgbEventKey_K,
//     MgbEventKey_L,
//     MgbEventKey_Z,
//     MgbEventKey_X,
//     MgbEventKey_C,
//     MgbEventKey_V,
//     MgbEventKey_B,
//     MgbEventKey_N,
//     MgbEventKey_M,

//     MgbEventKey_0,
//     MgbEventKey_1,
//     MgbEventKey_2,
//     MgbEventKey_3,
//     MgbEventKey_4,
//     MgbEventKey_5,
//     MgbEventKey_6,
//     MgbEventKey_7,
//     MgbEventKey_8,
//     MgbEventKey_9,

//     MgbEventKey_F1,
//     MgbEventKey_F2,
//     MgbEventKey_F3,
//     MgbEventKey_F4,
//     MgbEventKey_F5,
//     MgbEventKey_F6,
//     MgbEventKey_F7,
//     MgbEventKey_F8,
//     MgbEventKey_F9,
//     MgbEventKey_F10,
//     MgbEventKey_F11,
//     MgbEventKey_F12,

//     MgbEventKey_ENTER,
//     MgbEventKey_BACKSPACE,
//     MgbEventKey_SPACE,
//     MgbEventKey_TAB, // todo:
//     MgbEventKey_ESCAPE,
//     MgbEventKey_DELETE,
//     MgbEventKey_LSHIFT,
//     MgbEventKey_RSHIFT,
//     MgbEventKey_LCTRL, // todo:
//     MgbEventKey_RCTRL, // todo:

//     MgbEventKey_UP,
//     MgbEventKey_DOWN,
//     MgbEventKey_LEFT,
//     MgbEventKey_RIGHT,

//     MgbEventKey_MAX,
// };

// // based on SDL1 KeyMod enum!
// enum MgbEventKeyMod {
//     MgbEventKeyMod_NONE   = 0,
//     MgbEventKeyMod_LSHIFT = 1 << 0,
//     MgbEventKeyMod_RSHIFT = 1 << 1,
//     MgbEventKeyMod_LCTRL  = 1 << 2,
//     MgbEventKeyMod_RCTRL  = 1 << 3,
//     MgbEventKeyMod_LALT   = 1 << 4,
//     MgbEventKeyMod_RALT   = 1 << 5,
//     MgbEventKeyMod_NUM    = 1 << 6,
//     MgbEventKeyMod_CAPS   = 1 << 7,

//     // helpers for if we don't care if L/R side key is pressed
//     MgbEventKeyMod_SHIFT  = MgbEventKeyMod_LSHIFT | MgbEventKeyMod_RSHIFT,
//     MgbEventKeyMod_CTRL   = MgbEventKeyMod_LCTRL | MgbEventKeyMod_RCTRL,
//     MgbEventKeyMod_ALT    = MgbEventKeyMod_LALT | MgbEventKeyMod_RALT,
// };

// // maybe this should be a struct, with a bool for error
// // then a char[] for a string containing a error message...
// enum MgbEventQuitReason {
//     MgbEventQuitReason_ERROR,
//     MgbEventQuitReason_DEFAULT,
// };

// struct MgbEventInfo {
//     char window_name[256];
//     int x, y, w, h;
// };

// struct MgbEventGameTexture {
//     const void* pixels;
//     int pitch;
//     int w, h;
// };

// enum MgbEventType {
//     MgbEventType_FILE_DROP,
//     MgbEventType_MBUTTON,
//     MgbEventType_MMOTION,
//     MgbEventType_KEY,
//     MgbEventType_CBUTTON,
//     MgbEventType_CAXIS,
//     MgbEventType_RESIZE,
//     MgbEventType_HIDDEN,
//     MgbEventType_SHOWN,
//     MgbEventType_QUIT,
// };

// struct MgbEventDataFileDrop {
//     enum MgbEventType type;
//     const char* path;
// };

// struct MgbEventDataMouseButton {
//     enum MgbEventType type;
//     enum MgbEventMouseButton button;
//     int x, y;
//     bool down;
// };

// struct MgbEventDataMouseMotion {
//     enum MgbEventType type;
//     int x, y;
//     int xrel, yrel;
// };

// struct MgbEventDataKey {
//     enum MgbEventType type;
//     enum MgbEventKey key;
//     uint16_t mod;
//     bool down;
// };

// struct MgbEventDataControllerButton {
//     enum MgbEventType type;
//     enum MgbEventControllerButton button;
//     bool down;
// };

// struct MgbEventDataControllerAxis {
//     enum MgbEventType type;
//     enum MgbEventControllerAxis axis;
//     int16_t pos;
//     bool down;
// };

// struct MgbEventDataResize {
//     enum MgbEventType type;
//     int w, h;
//     int display_w, display_h;
// };

// struct MgbEventDataHidden {
//     enum MgbEventType type;
// };

// struct MgbEventDataShown {
//     enum MgbEventType type;
// };

// struct MgbEventDataQuit {
//     enum MgbEventType type;
//     enum MgbEventQuitReason reason;
// };

// // very much inspired by SDL2 event
// union MgbEvent {
//     enum MgbEventType type;

//     struct MgbEventDataFileDrop file_drop;
//     struct MgbEventDataMouseButton mbutton;
//     struct MgbEventDataMouseMotion mmotion;
//     struct MgbEventDataKey key;
//     struct MgbEventDataControllerButton cbutton;
//     struct MgbEventDataControllerAxis caxis;
//     struct MgbEventDataResize resize;
//     struct MgbEventDataHidden hidden;
//     struct MgbEventDataShown shown;
//     struct MgbEventDataQuit quit;
// };

// void mgb_event_process(const union MgbEvent* e);

#ifdef __cplusplus
}
#endif
