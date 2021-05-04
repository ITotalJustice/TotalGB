#include "mgb.h"
#include "romloader.h"
#include "ifile/cfile/cfile.h"
#include "ifile/gzip/gzip.h"

#ifdef MGB_VIDEO_BACKEND_SDL1
    #include "video/sdl1/sdl1.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl1
#elif MGB_VIDEO_BACKEND_SDL1_OPENGL
    #include "video/sdl1/sdl1_opengl.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl1_opengl
    #define MGB_GUI 1
#elif MGB_VIDEO_BACKEND_SDL2
    #include "video/sdl2/sdl2.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2
#elif MGB_VIDEO_BACKEND_SDL2_OPENGL
    #include "video/sdl2/sdl2_opengl.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2_opengl
    #define MGB_GUI 1
#elif MGB_VIDEO_BACKEND_SDL2_VULKAN
    #include "video/sdl2/sdl2_vulkan.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2_vulkan
#elif MGB_VIDEO_BACKEND_ALLEGRO4
    #include "video/allegro4/allegro4.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_allegro4
#elif MGB_VIDEO_BACKEND_ALLEGRO5
    #include "video/allegro5/allegro5.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_allegro5
#else
    #error "NO VIDEO BACKEND SELECTED FOR MGB!"
#endif

// only enabled with opengl backends!
// can technically also support vulkan, but i dont have a vulkan pc to test
// this will also (soon) be supported when using sdl2 renderer
//  SEE: https://github.com/libsdl-org/SDL/pull/4195
//  SEE: https://github.com/Immediate-Mode-UI/Nuklear/pull/280
#ifndef MGB_GUI
#define MGB_GUI 0
#endif

#if MGB_GUI
    // todo:
#endif

// todo: filter audio includes
#include "audio/sdl2/sdl2.h"
#include "audio/sdl1/sdl1.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


enum LoadRomType {
    LoadRomType_FILE,
    LoadRomType_MEM,
};

struct LoadRomConfig {
    const char* const path;
    const uint8_t* const data;
    const size_t size;
    enum LoadRomType type;
};


// [VIDEO INSTANCE CALLBACKS]
static void on_file_drop(void* user,
    const char* path
);
static void on_key(void* user,
    enum VideoInterfaceKey key, bool down
);
static void on_button(void* user,
    enum VideoInterfaceButton button, bool down
);
static void on_axis(void* user,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);
static void on_quit(void* user,
    enum VideoInterfaceQuitReason reason
);


// [CORE CALLBACKS]
static void core_on_apu(struct GB_Core* gb,
    void* user, const struct GB_ApuCallbackData* data
);
static void core_on_error(struct GB_Core* gb,
    void* user, struct GB_ErrorData* e
);
static void core_on_vsync(struct GB_Core* gb,
    void* user
);
static void core_on_hblank(struct GB_Core* gb,
    void* user
);
static void core_on_dma(struct GB_Core* gb,
    void* user
);
static void core_on_halt(struct GB_Core* gb,
    void* user
);
static void core_on_stop(struct GB_Core* gb,
    void* user
);

static bool setup_video_interface(mgb_t* self);
static bool setup_audio_interface(mgb_t* self);
static bool setup_core(mgb_t* self);

static bool loadrom(mgb_t* self, const struct LoadRomConfig* ctx);
static void free_game(mgb_t* self);
static void savegame(mgb_t* self);
static void loadsave(mgb_t* self);


static void free_game(mgb_t* self) {
    if (!self->rom_loaded) {
        assert(self->rom_data == NULL);
        return;
    }

    assert(self->rom_data != NULL);

    // save the game before exiting
    savegame(self);

    free(self->rom_data);
    self->rom_data = NULL;
    self->rom_loaded = false;
}

static void savegame(mgb_t* self) {
    if (!self->rom_loaded) {
        return;
    }

    const bool game_has_save = GB_has_save(self->gameboy);
    const bool game_has_rtc = GB_has_rtc(self->gameboy);

    if (game_has_save || game_has_rtc) {
        // get the paths
        const struct SafeString save_path = util_create_save_path(
            self->rom_path.str
        );

        const struct SafeString rtc_path = util_create_rtc_path(
            self->rom_path.str
        );

        struct GB_SaveData save_data = {0};
        GB_savegame(self->gameboy, &save_data);

        if (game_has_save && save_path.valid) {
            const size_t save_size = GB_calculate_savedata_size(self->gameboy);

            IFile_t* file = icfile_open(save_path.str, "wb");
            
            if (file) {
                if (ifile_write(file, save_data.data, save_size)) {
                }
            }

            ifile_close(file);
        }

        if (game_has_rtc && rtc_path.valid) {
            IFile_t* file = icfile_open(rtc_path.str, "wb");
            
            if (file) {
                if (ifile_write(file, &save_data.rtc, sizeof(save_data.rtc))) {
                }
            }

            ifile_close(file);
        }
    }
}

static void loadsave(mgb_t* self) {
    const bool game_has_save = GB_has_save(self->gameboy);
    const bool game_has_rtc = GB_has_rtc(self->gameboy);

    if (game_has_save || game_has_rtc) {
        // get the paths
        const struct SafeString save_path = util_create_save_path(
            self->rom_path.str
        );

        const struct SafeString rtc_path = util_create_rtc_path(
            self->rom_path.str
        );

        struct GB_SaveData save_data = {0};

        if (game_has_save && save_path.valid) {
            const size_t save_size = GB_calculate_savedata_size(self->gameboy);

            IFile_t* file = icfile_open(save_path.str, "rb");
            
            if (file) {
                if (ifile_size(file) == save_size) {
                    if (ifile_read(file, save_data.data, save_size)) {
                        save_data.size = save_size;
                    }
                }
            }

            ifile_close(file);
        }

        if (game_has_rtc && rtc_path.valid) {
            const size_t save_size = sizeof(save_data.rtc);

            IFile_t* file = icfile_open(rtc_path.str, "rb");
            
            if (file) {
                if (ifile_size(file) == save_size) {
                    if (ifile_read(file, &save_data.rtc, save_size)) {
                        save_data.has_rtc = true;
                    }
                }
            }

            ifile_close(file);
        }

        GB_loadsave(self->gameboy, &save_data);
    }
}

// [CORE CALLBACKS]
static void core_on_apu(struct GB_Core* gb,
    void* user, const struct GB_ApuCallbackData* data
) {
    (void)gb; (void)user; (void)data;
}

static void core_on_error(struct GB_Core* gb,
    void* user, struct GB_ErrorData* e
) {
    (void)gb; (void)user;

    switch (e->type) {
        case GB_ERROR_TYPE_UNKNOWN_INSTRUCTION:
            if (e->data.unk_instruction.cb_prefix) {
                printf("[ERROR] UNK Opcode 0xCB%02X\n", e->data.unk_instruction.opcode);
            }
            else {
                printf("[ERROR] UNK Opcode 0x%02X\n", e->data.unk_instruction.opcode);
            }
            break;

        case GB_ERROR_TYPE_INFO:
            printf("[INFO] %s\n", e->data.info.message);
            break;

        case GB_ERROR_TYPE_WARN:
            printf("[WARN] %s\n", e->data.warn.message);
            break;

        case GB_ERROR_TYPE_ERROR:
            printf("[ERROR] %s\n", e->data.error.message);
            break;

        case GB_ERROR_TYPE_UNK:
            printf("[ERROR] Unknown gb error...\n");
            break;
    }
}

static void core_on_vsync(struct GB_Core* gb,
    void* user
) {
    mgb_t* self = (mgb_t*)user;

    const struct VideoInterfaceGameTexture game_texture = {
        .pixels = (uint16_t*)gb->ppu.pixles
    };

    video_interface_update_game_texture(
        self->video_interface, &game_texture
    );
}

static void core_on_hblank(struct GB_Core* gb,
    void* user
) {
    (void)gb; (void)user;
}

static void core_on_dma(struct GB_Core* gb,
    void* user
) {
    (void)gb; (void)user;
}

static void core_on_halt(struct GB_Core* gb,
    void* user
) {
    (void)gb; (void)user;
}

static void core_on_stop(struct GB_Core* gb,
    void* user
) {
    (void)gb; (void)user;
}


// [VIDEO INTERFACE CALLBACKS]
static void on_file_drop(void* user,
    const char* path
) {
    (void)user; (void)path;
}

static void on_key(void* user,
    enum VideoInterfaceKey key, bool down
) {
    mgb_t* self = (mgb_t*)user;

    // todo: check which state we are in for which map to use
    // ie, for ui state, use the ui key map, for core, use core map etc..

    // todo: make this customisable by user
    static const enum VideoInterfaceKey map[VideoInterfaceKey_MAX] = {
        [VideoInterfaceKey_Z]       = GB_BUTTON_B,
        [VideoInterfaceKey_X]       = GB_BUTTON_A,
        [VideoInterfaceKey_ENTER]   = GB_BUTTON_START,
        [VideoInterfaceKey_SPACE]   = GB_BUTTON_SELECT,
        [VideoInterfaceKey_UP]      = GB_BUTTON_UP,
        [VideoInterfaceKey_DOWN]    = GB_BUTTON_DOWN,
        [VideoInterfaceKey_LEFT]    = GB_BUTTON_LEFT,
        [VideoInterfaceKey_RIGHT]   = GB_BUTTON_RIGHT,
    };

    if (map[key]) {
        GB_set_buttons(self->gameboy, map[key], down);
    }
}

static void on_button(void* user,
    enum VideoInterfaceButton button, bool down
) {
    mgb_t* self = (mgb_t*)user;

    static const enum VideoInterfaceButton map[VideoInterfaceButton_MAX] = {
        [VideoInterfaceButton_B]        = GB_BUTTON_B,
        [VideoInterfaceButton_A]        = GB_BUTTON_A,
        [VideoInterfaceButton_START]    = GB_BUTTON_START,
        [VideoInterfaceButton_SELECT]   = GB_BUTTON_SELECT,
        [VideoInterfaceButton_UP]       = GB_BUTTON_UP,
        [VideoInterfaceButton_DOWN]     = GB_BUTTON_DOWN,
        [VideoInterfaceButton_LEFT]     = GB_BUTTON_LEFT,
        [VideoInterfaceButton_RIGHT]    = GB_BUTTON_RIGHT,
    };

    if (map[button]) {
        GB_set_buttons(self->gameboy, map[button], down);
    }
}

static void on_axis(void* user,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
) {
    (void)user; (void)axis; (void)pos; (void)down;
}

static void on_quit(void* user,
    enum VideoInterfaceQuitReason reason
) {
    (void)reason;

    mgb_t* self = (mgb_t*)user;

    self->running = false;
}

static bool setup_video_interface(mgb_t* self) {
    if (self->video_interface) {
        video_interface_quit(self->video_interface);
        self->video_interface = NULL;
    }

    const struct VideoInterfaceInfo info = {
        .window_name = "Hello, World!",
        .x = 0,
        .y = 0,
        .w = 160 * 2,
        .h = 144 * 2,
    };

    const struct VideoInterfaceUserCallbacks callbacks = {
        .user = self,
        .on_file_drop = on_file_drop,
        .on_key = on_key,
        .on_button = on_button,
        .on_axis = on_axis,
        .on_quit = on_quit
    };

    self->video_interface = VIDEO_INTERFACE_INIT(
        &info, &callbacks
    );

    return self->video_interface != NULL;
}

static bool setup_audio_interface(mgb_t* self) {
    if (self->audio_interface) {
        // audio_interface_quit(self->audio_interface);
        self->audio_interface = NULL;
    }

    return true;

    // return self->audio_interface != NULL;
}

static bool setup_core(mgb_t* self) {
    if (self->gameboy) {
        free(self->gameboy);
        self->gameboy = NULL;
    }

    self->gameboy = malloc(sizeof(struct GB_Core));

    if (!self->gameboy) {
        goto fail;
    }

    if (!GB_init(self->gameboy)) {
        goto fail;
    }

    GB_set_rtc_update_config(
        self->gameboy, GB_RTC_UPDATE_CONFIG_USE_LOCAL_TIME
    );

    // GB_set_apu_callback(self->gameboy, core_on_vsync, self);
    (void)core_on_apu;
    GB_set_vblank_callback(self->gameboy, core_on_vsync, self);
    GB_set_hblank_callback(self->gameboy, core_on_hblank, self);
    GB_set_dma_callback(self->gameboy, core_on_dma, self);
    GB_set_halt_callback(self->gameboy, core_on_halt, self);
    GB_set_stop_callback(self->gameboy, core_on_stop, self);
    GB_set_error_callback(self->gameboy, core_on_error, self);

    return true;

fail:
    if (self->gameboy) {
        free(self->gameboy);
        self->gameboy = NULL;
    }

    return false;
}

static bool loadrom(mgb_t* self, const struct LoadRomConfig* ctx) {
    IFile_t* romloader = NULL;

    if (self->rom_loaded) {
        free_game(self);
    }

    switch (ctx->type) {
        case LoadRomType_FILE:
            romloader = romloader_open(ctx->path);
            break;

        case LoadRomType_MEM:
            printf("loading rom from mem not implemented yet!\n");
            exit(-1);
            break;
    }

    if (!romloader) {
        goto fail;
    }

    self->rom_size = ifile_size(romloader);

    if (!self->rom_size || self->rom_size > 0x400000) {
        printf("size is too big\n");
        goto fail;
    }

    self->rom_data = malloc(self->rom_size);

    if (!self->rom_data) {
        printf("fail to alloc\n");
        goto fail;
    }

    if (!ifile_read(romloader, self->rom_data, self->rom_size)) {
        printf("fail to read size: %zu\n", self->rom_size);
        goto fail;
    }

    if (!GB_loadrom(self->gameboy, self->rom_data, self->rom_size)) {
        printf("fail to gb load rom\n");
        goto fail;
    }

    // free everything
    ifile_close(romloader);
    romloader = NULL;

    // save the path
    strncpy(self->rom_path.str, ctx->path, sizeof(self->rom_path.str) - 1);
    self->rom_loaded = true;

    // try loading any saves if possible
    loadsave(self);

    return true;

fail:
    if (romloader) {
        ifile_close(romloader);
        romloader = NULL;
    }

    return false;
}

bool mgb_init(mgb_t* self) {
    if (!self) {
        return false;
    }

    memset(self, 0, sizeof(struct mgb));

    if (!setup_core(self)) {
        goto fail;
    }

    if (!setup_video_interface(self)) {
        goto fail;
    }

    if (!setup_audio_interface(self)) {
        goto fail;
    }

    self->running = true;
    return true;

fail:
    mgb_exit(self);

    return false;
}

void mgb_exit(mgb_t* self) {
    if (!self) {
        return;
    }

    free_game(self);

    if (self->gameboy) {
        free(self->gameboy);
        self->gameboy = NULL;
    }

    if (self->video_interface) {
        video_interface_quit(self->video_interface);
        self->video_interface = NULL;
    }

    if (self->audio_interface) {
        // audio_interface_quit(self->audio_interface);
        self->audio_interface = NULL;
    }
}

void mgb_loop(mgb_t* self) {
    while (self->running == true) {
        video_interface_poll_events(self->video_interface);

        if (self->rom_loaded) {
            GB_run_frame(self->gameboy);
        }

        video_interface_render_begin(self->video_interface);
        video_interface_render_game(self->video_interface);
        video_interface_render_end(self->video_interface);
    }
}

bool mgb_load_rom_file(mgb_t* self, const char* path) {
    const struct LoadRomConfig ctx = {
        .path = path,
        .data = NULL,
        .size = 0,
        .type = LoadRomType_FILE
    };

    return loadrom(self, &ctx);
}

bool mgb_load_rom_data(mgb_t* self,
    const char* path, const uint8_t* data, size_t size
) {
    const struct LoadRomConfig ctx = {
        .path = path,
        .data = data,
        .size = size,
        .type = LoadRomType_MEM
    };

    return loadrom(self, &ctx);
}
