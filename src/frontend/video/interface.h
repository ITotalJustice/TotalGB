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

enum VideoInterfaceButton {
    VideoInterfaceButton_A = 1,
    VideoInterfaceButton_B,
    VideoInterfaceButton_X,
    VideoInterfaceButton_Y,

    VideoInterfaceButton_START,
    VideoInterfaceButton_SELECT,
    VideoInterfaceButton_HOME,

    VideoInterfaceButton_L1,
    VideoInterfaceButton_L3,
    VideoInterfaceButton_R1,
    VideoInterfaceButton_R3,

    VideoInterfaceButton_UP,
    VideoInterfaceButton_DOWN,
    VideoInterfaceButton_LEFT,
    VideoInterfaceButton_RIGHT,

    VideoInterfaceButton_MAX
};

enum VideoInterfaceAxis {
    VideoInterfaceAxis_L2 = 1,
    VideoInterfaceAxis_R2,
    VideoInterfaceAxis_LEFTX,
    VideoInterfaceAxis_LEFTY,
    VideoInterfaceAxis_RIGHTX,
    VideoInterfaceAxis_RIGHTY,

    VideoInterfaceAxis_MAX,
};

// not yes finished!
enum VideoInterfaceKey {
    VideoInterfaceKey_Q = 1,
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
    VideoInterfaceKey_ESCAPE,
    VideoInterfaceKey_DELETE,
    VideoInterfaceKey_LSHIFT,
    VideoInterfaceKey_RSHIFT,

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

struct VideoInterfaceUserCallbacks {
    void* user;

    void (*on_file_drop)(void* user,
        const char* path
    );
    void (*on_key)(void* user,
        enum VideoInterfaceKey key, uint8_t mod, bool down
    );
    void (*on_button)(void* user,
        enum VideoInterfaceButton button, bool down
    );
    void (*on_axis)(void* user,
        enum VideoInterfaceAxis axis, int16_t pos, bool down
    );
    void (*on_resize)(void* user,
        int w, int h
    );
    void (*on_quit)(void* user,
        enum VideoInterfaceQuitReason reason
    );
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

    // these are set by the interface after init!
    int x, y, w, h;

    bool has_file_dialog;
    bool has_message_box;
    bool has_controller;
    bool has_keyboard;
    bool has_mouse;

    bool is_vsync;
    bool is_hw_accel;
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

void video_interface_get_dimensions(
    const struct VideoInterface* self,
    int* x, int* y, int* w, int* h
);

bool video_interface_has_file_dialog(
    const struct VideoInterface* self
);

bool video_interface_has_message_box(
    const struct VideoInterface* self
);

bool video_interface_has_controller(
    const struct VideoInterface* self
);

bool video_interface_has_keyboard(
    const struct VideoInterface* self
);

bool video_interface_has_mouse(
    const struct VideoInterface* self
);

bool video_interface_is_vsync(
    const struct VideoInterface* self
);

bool video_interface_is_hw_accel(
    const struct VideoInterface* self
);

#ifdef __cplusplus
}
#endif
