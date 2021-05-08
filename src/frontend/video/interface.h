#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


// fwd
struct VideoInterface;
struct VideoInterfaceGameTexture;
struct VideoInterfaceUserCallbacks;

enum VideoInterfaceMouseButton {
    VideoInterfaceMouseButton_NONE = 0,

    VideoInterfaceMouseButton_LEFT,
    VideoInterfaceMouseButton_MIDDLE,
    VideoInterfaceMouseButton_RIGHT,

    VideoInterfaceMouseButton_MAX,
};

enum VideoInterfaceControllerButton {
    VideoInterfaceControllerButton_NONE = 0,

    VideoInterfaceControllerButton_A,
    VideoInterfaceControllerButton_B,
    VideoInterfaceControllerButton_X,
    VideoInterfaceControllerButton_Y,

    VideoInterfaceControllerButton_START,
    VideoInterfaceControllerButton_SELECT,
    VideoInterfaceControllerButton_HOME,

    VideoInterfaceControllerButton_L1,
    VideoInterfaceControllerButton_L3,
    VideoInterfaceControllerButton_R1,
    VideoInterfaceControllerButton_R3,

    VideoInterfaceControllerButton_UP,
    VideoInterfaceControllerButton_DOWN,
    VideoInterfaceControllerButton_LEFT,
    VideoInterfaceControllerButton_RIGHT,

    VideoInterfaceControllerButton_MAX
};

enum VideoInterfaceControllerAxis {
    VideoInterfaceControllerAxis_NONE = 0,

    VideoInterfaceControllerAxis_L2,
    VideoInterfaceControllerAxis_R2,
    VideoInterfaceControllerAxis_LEFTX,
    VideoInterfaceControllerAxis_LEFTY,
    VideoInterfaceControllerAxis_RIGHTX,
    VideoInterfaceControllerAxis_RIGHTY,

    VideoInterfaceControllerAxis_MAX,
};

// not yes finished!
enum VideoInterfaceKey {
    VideoInterfaceKey_NONE = 0,

    VideoInterfaceKey_Q,
    VideoInterfaceKey_W,
    VideoInterfaceKey_E,
    VideoInterfaceKey_R,
    VideoInterfaceKey_T,
    VideoInterfaceKey_Y,
    VideoInterfaceKey_U,
    VideoInterfaceKey_I,
    VideoInterfaceKey_O,
    VideoInterfaceKey_P,
    VideoInterfaceKey_A,
    VideoInterfaceKey_S,
    VideoInterfaceKey_D,
    VideoInterfaceKey_F,
    VideoInterfaceKey_G,
    VideoInterfaceKey_H,
    VideoInterfaceKey_J,
    VideoInterfaceKey_K,
    VideoInterfaceKey_L,
    VideoInterfaceKey_Z,
    VideoInterfaceKey_X,
    VideoInterfaceKey_C,
    VideoInterfaceKey_V,
    VideoInterfaceKey_B,
    VideoInterfaceKey_N,
    VideoInterfaceKey_M,

    VideoInterfaceKey_0,
    VideoInterfaceKey_1,
    VideoInterfaceKey_2,
    VideoInterfaceKey_3,
    VideoInterfaceKey_4,
    VideoInterfaceKey_5,
    VideoInterfaceKey_6,
    VideoInterfaceKey_7,
    VideoInterfaceKey_8,
    VideoInterfaceKey_9,

    VideoInterfaceKey_F1,
    VideoInterfaceKey_F2,
    VideoInterfaceKey_F3,
    VideoInterfaceKey_F4,
    VideoInterfaceKey_F5,
    VideoInterfaceKey_F6,
    VideoInterfaceKey_F7,
    VideoInterfaceKey_F8,
    VideoInterfaceKey_F9,
    VideoInterfaceKey_F10,
    VideoInterfaceKey_F11,
    VideoInterfaceKey_F12,

    VideoInterfaceKey_ENTER,
    VideoInterfaceKey_BACKSPACE,
    VideoInterfaceKey_SPACE,
    VideoInterfaceKey_TAB, // todo:
    VideoInterfaceKey_ESCAPE,
    VideoInterfaceKey_DELETE,
    VideoInterfaceKey_LSHIFT,
    VideoInterfaceKey_RSHIFT,
    VideoInterfaceKey_LCTRL, // todo:
    VideoInterfaceKey_RCTRL, // todo:

    VideoInterfaceKey_UP,
    VideoInterfaceKey_DOWN,
    VideoInterfaceKey_LEFT,
    VideoInterfaceKey_RIGHT,

    VideoInterfaceKey_MAX,
};

// based on SDL1 KeyMod enum!
enum VideoInterfaceKeyMod {
    VideoInterfaceKeyMod_NONE   = 0,
    VideoInterfaceKeyMod_LSHIFT = 1 << 0,
    VideoInterfaceKeyMod_RSHIFT = 1 << 1,
    VideoInterfaceKeyMod_LCTRL  = 1 << 2,
    VideoInterfaceKeyMod_RCTRL  = 1 << 3,
    VideoInterfaceKeyMod_LALT   = 1 << 4,
    VideoInterfaceKeyMod_RALT   = 1 << 5,
    VideoInterfaceKeyMod_NUM    = 1 << 6,
    VideoInterfaceKeyMod_CAPS   = 1 << 7,

    // helpers for if we don't care if L/R side key is pressed
    VideoInterfaceKeyMod_SHIFT  = VideoInterfaceKeyMod_LSHIFT | VideoInterfaceKeyMod_RSHIFT,
    VideoInterfaceKeyMod_CTRL   = VideoInterfaceKeyMod_LCTRL | VideoInterfaceKeyMod_RCTRL,
    VideoInterfaceKeyMod_ALT    = VideoInterfaceKeyMod_LALT | VideoInterfaceKeyMod_RALT,
};

// maybe this should be a struct, with a bool for error
// then a char[] for a string containing a error message...
enum VideoInterfaceQuitReason {
    VideoInterfaceQuitReason_ERROR,
    VideoInterfaceQuitReason_DEFAULT,
};

struct VideoInterfaceInfo {
    char window_name[256];
    int x, y, w, h;
};

struct VideoInterfaceGameTexture {
    const uint16_t* pixels;
    int w, h;
};

enum VideoInterfaceEventType {
    VideoInterfaceEventType_FILE_DROP,
    VideoInterfaceEventType_MBUTTON,
    VideoInterfaceEventType_MMOTION,
    VideoInterfaceEventType_KEY,
    VideoInterfaceEventType_CBUTTON,
    VideoInterfaceEventType_CAXIS,
    VideoInterfaceEventType_RESIZE,
    VideoInterfaceEventType_HIDDEN,
    VideoInterfaceEventType_SHOWN,
    VideoInterfaceEventType_QUIT,
};

struct VideoInterfaceEventDataFileDrop {
    enum VideoInterfaceEventType type;
    const char* path;
};

struct VideoInterfaceEventDataMouseButton {
    enum VideoInterfaceEventType type;
    enum VideoInterfaceMouseButton button;
    int x, y;
    bool down;
};

struct VideoInterfaceEventDataMouseMotion {
    enum VideoInterfaceEventType type;
    int x, y;
    int xrel, yrel;
};

struct VideoInterfaceEventDataKey {
    enum VideoInterfaceEventType type;
    enum VideoInterfaceKey key;
    uint16_t mod;
    bool down;
};

struct VideoInterfaceEventDataControllerButton {
    enum VideoInterfaceEventType type;
    enum VideoInterfaceControllerButton button;
    bool down;
};

struct VideoInterfaceEventDataControllerAxis {
    enum VideoInterfaceEventType type;
    enum VideoInterfaceControllerAxis axis;
    int16_t pos;
    bool down;
};

struct VideoInterfaceEventDataResize {
    enum VideoInterfaceEventType type;
    int w, h;
    int display_w, display_h;
};

struct VideoInterfaceEventDataHidden {
    enum VideoInterfaceEventType type;
};

struct VideoInterfaceEventDataShown {
    enum VideoInterfaceEventType type;
};

struct VideoInterfaceEventDataQuit {
    enum VideoInterfaceEventType type;
    enum VideoInterfaceQuitReason reason;
};

// very much inspired by SDL2 event
union VideoInterfaceEvent {
    enum VideoInterfaceEventType type;

    struct VideoInterfaceEventDataFileDrop file_drop;
    struct VideoInterfaceEventDataMouseButton mbutton;
    struct VideoInterfaceEventDataMouseMotion mmotion;
    struct VideoInterfaceEventDataKey key;
    struct VideoInterfaceEventDataControllerButton cbutton;
    struct VideoInterfaceEventDataControllerAxis caxis;
    struct VideoInterfaceEventDataResize resize;
    struct VideoInterfaceEventDataHidden hidden;
    struct VideoInterfaceEventDataShown shown;
    struct VideoInterfaceEventDataQuit quit;
};

struct VideoInterface {
    void* _private;

    void (*quit)(
        void* _private
    );
    void (*render_begin)(
        void* _private
    );
    void (*render_game)(
        void* _private
    );
    void (*render_end)(
        void* _private
    );
    void (*update_game_texture)(
        void* _private,
        const struct VideoInterfaceGameTexture* game_texture
    );
    void (*poll_events)(
        void* _private
    );
    void (*toggle_fullscreen)(
        void* _private
    );
    void (*set_window_name)(
        void* _private,
        const char* name
    );
};


void video_interface_quit(
    struct VideoInterface* self
);

void video_interface_render_begin(
    struct VideoInterface* self
);

void video_interface_render_game(
    struct VideoInterface* self
);

void video_interface_render_end(
    struct VideoInterface* self
);

void video_interface_update_game_texture(
    struct VideoInterface* self,
    const struct VideoInterfaceGameTexture* game_texture
);

void video_interface_poll_events(
    struct VideoInterface* self
);

void video_interface_toggle_fullscreen(
    struct VideoInterface* self
);

void video_interface_set_window_name(
    struct VideoInterface* self,
    const char* name
);

#ifdef __cplusplus
}
#endif
