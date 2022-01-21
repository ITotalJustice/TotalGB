#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL.h>
#include <gb.h>

#define VERSION "v0.0.1e"
#define VERSION_STRING "Welcome to TotalGB - " VERSION

enum
{
#ifdef EMSCRIPTEN
    DEFAULT_SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_AUDIO,
#else
    DEFAULT_SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER,
#endif
    DEFAULT_SYNC_VSYNC = 1,
    DEFAULT_SPEED = 1,
    DEFAULT_SCALE = 4,

#if 0
    RENDERER_FLAGS = SDL_RENDERER_ACCELERATED,
#else
    RENDERER_FLAGS = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC,
#endif

    WIDTH = GB_SCREEN_WIDTH,
    HEIGHT = GB_SCREEN_HEIGHT,
};

enum MenuType
{
    MenuType_ROM,
    MenuType_SIDEBAR,
};

typedef struct
{
    struct GB_Core gb;
    enum MenuType menu_type;
    int speed;
    int scale;
    bool running;
    bool vsync;
    bool pending_frame;
    bool rendered_frame;
    bool rewinding;
} emu_t;

#if 1
    #define log_info(...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
    #define log_error(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#else
    #define log_info(...)
    #define log_error(...)
#endif

void on_title_click(void);

enum MenuType get_menu_type(void);
void change_menu(enum MenuType new_menu);

#ifdef ANDROID
void android_file_picker(void);
void android_settings_menu(void);
void android_folder_picker(void);
#endif

void scale_screen(int new_scale);
void set_speed(int x);
int get_scale(int w, int h);

void toggle_vsync(void);
void set_fullscreen(bool);
void toggle_fullscreen(void);
bool is_fullscreen(void);

void savestate(const char* path);
void loadstate(const char* path);

// whoever locks this has full access to the core
void set_emu_button(uint8_t gb_button, bool down);
void lock_core(void);
void unlock_core(void);

#ifdef __cplusplus
}
#endif
