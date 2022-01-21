#include "mgb.h"
#include "romloader.h"
#include "filedialog.h"
#include "sdl_helper.h"
#include "util.h"
#include "ifile/cfile/cfile.h"
#include "ifile/gzip/gzip.h"
#include "ifile/mem/mem.h"

#include <stdio.h>
#include <string.h>
#include <gb.h>
#include <assert.h>

#ifdef HAS_SDL2
    #include <SDL.h>
#endif

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

#ifdef HAS_SDL2
    #define mgb_log(...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
    #define mgb_log_err(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#else
    #define mgb_log(...) fprintf(stdout, __VA_ARGS__)
    #define mgb_log_err(...) fprintf(stderr, __VA_ARGS__)
#endif

enum LoadRomType
{
    LoadRomType_FILE,
    LoadRomType_MEM,
    LoadRomType_FD,
};

struct LoadRomConfig
{
    const char* const path;
    const uint8_t* const data;
    const size_t size;
    enum LoadRomType type;
    int fd;
    bool own_fd;
};

struct RewindState
{
    struct GB_State state;
    uint32_t pixels[144][160];
};

struct mgb
{
    // set via init()
    struct GB_Core* gb;

    void (*on_file_cb)(const char*, enum CallbackType, bool);

    // [OPTIONAL]
    // folder prefixes, eg, save_folder = "/saves";
    // then the full path is /saves/rom_name.sav
    char* save_folder;
    char* rtc_folder;
    char* state_folder;

    // copy of state is here so we don't waste time
    // malloc / dealloc
    struct GB_State state;

    char rom_path[0x304];
    uint8_t rom_data[GB_ROM_SIZE_MAX];
    size_t rom_size;
    bool has_rom;

    uint8_t sram_data[GB_SAVE_SIZE_MAX];
    size_t sram_size;
    bool has_sram;

    bool rom_loaded;
};


static void free_game(void);
static void loadsave(void);
static bool loadrom(const struct LoadRomConfig* config);

// globals
static struct mgb mgb = {0};
// globals end

static void free_game(void)
{
    mgb_save_save_file(NULL);
    // mgb_save_state_file(NULL);
    mgb.rom_loaded = false;
}

static void loadsave(void)
{
    if (GB_has_save(mgb.gb))
    {
        const struct SafeString ss = util_create_save_path(mgb.save_folder, mgb.rom_path);

        if (ss_valid(&ss))
        {
            IFile_t* file = icfile_open(ss.str, IFileMode_READ, 0);

            if (file)
            {
                const size_t save_size = GB_calculate_savedata_size(mgb.gb);

                // we allow for bigger sizes because it might be an vba save
                if (ifile_size(file) >= save_size && save_size < sizeof(mgb.sram_data))
                {
                    if (ifile_read(file, mgb.sram_data, save_size))
                    {
                        mgb_log("[MGB] loaded save: %s\n", ss.str);
                    }
                    else
                    {
                        mgb_log_err("[MGB] failed to load save: %s\n", ss.str);
                    }
                }
                else
                {
                    mgb_log_err("[MGB] failed to load save: %s invalid size\n", ss.str);
                }

                ifile_close(file);
            }
        }
    }

    // we always set sram!
    GB_set_sram(mgb.gb, mgb.sram_data, sizeof(mgb.sram_data));
}

static bool loadrom(const struct LoadRomConfig* config)
{
    IFile_t* romloader = NULL;

    if (mgb_has_rom())
    {
        free_game();
    }

    switch (config->type)
    {
        case LoadRomType_FILE:
            romloader = romloader_open(config->path);
            break;

        case LoadRomType_MEM:
            romloader = romloader_open_mem(config->path, config->data, config->size);
            break;

        case LoadRomType_FD:
            romloader = romloader_open_fd(config->fd, config->own_fd, config->path);
            break;
    }

    if (!romloader)
    {
        goto fail;
    }

    mgb.rom_size = ifile_size(romloader);

    if (!mgb.rom_size || mgb.rom_size > sizeof(mgb.rom_data))
    {
        mgb_log_err("[MGB] size is bad %zu\n", mgb.rom_size);
        goto fail;
    }

    if (!ifile_read(romloader, mgb.rom_data, mgb.rom_size))
    {
        mgb_log_err("[MGB] fail to read size: %zu\n", mgb.rom_size);
        goto fail;
    }

    // todo: move this somewhere more tidy
    GB_set_sram(mgb.gb, mgb.sram_data, sizeof(mgb.sram_data));

    if (!GB_loadrom(mgb.gb, mgb.rom_data, mgb.rom_size))
    {
        mgb_log_err("[MGB] fail to gb load rom\n");
        goto fail;
    }

    // free everything
    ifile_close(romloader);
    romloader = NULL;

    // save the path
    strncpy(mgb.rom_path, config->path, sizeof(mgb.rom_path) - 1);

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(config->path, CallbackType_LOAD_ROM, true);
    }

    // try loading any saves if possible
    loadsave();
    // mgb_load_state_file(NULL);
    mgb.rom_loaded = true;

    return true;

fail:
    if (romloader)
    {
        ifile_close(romloader);
        romloader = NULL;
    }

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(config->path, CallbackType_LOAD_ROM, false);
    }

    mgb.rom_loaded = false;

    return false;
}

bool mgb_load_rom_filedialog(void)
{
#ifdef EMSCRIPTEN
    EM_ASM(
        let rom_input = document.getElementById("RomFilePicker");
        rom_input.click();
    );
    return true;
#else
    const char* filters = "gb,gbc,zip";
    const struct FileDialogResult result = filedialog_open_file(filters);

    switch (result.type)
    {
        case FileDialogResultType_OK:
            return mgb_load_rom_file(result.path);

        case FileDialogResultType_ERROR:
            return false;

        case FileDialogResultType_CANCEL:
            return false;
    }

    return false;
#endif
}

bool mgb_save_state_filedialog(void)
{
    const char* const filters = "state";
    const struct FileDialogResult result = filedialog_save_file(filters);

    switch (result.type)
    {
        case FileDialogResultType_OK:
            return mgb_save_state_file(result.path);

        case FileDialogResultType_ERROR:
        case FileDialogResultType_CANCEL:
            return false;
    }

    return false;
}

bool mgb_load_state_filedialog(void)
{
    const char* const filters = "state";
    const struct FileDialogResult result = filedialog_open_file(filters);

    switch (result.type)
    {
        case FileDialogResultType_OK:
            return mgb_load_state_file(result.path);

        case FileDialogResultType_ERROR:
        case FileDialogResultType_CANCEL:
            return false;
    }

    return false;
}

bool mgb_load_rom_file(const char* path)
{
    const struct LoadRomConfig config =
    {
        .path = path,
        .type = LoadRomType_FILE
    };

    return loadrom(&config);
}

bool mgb_load_rom_fd(int fd, bool own, const char* path)
{
    const struct LoadRomConfig config =
    {
        .path = path,
        .type = LoadRomType_FD,
        .fd = fd,
        .own_fd = own,
    };

    return loadrom(&config);
}

bool mgb_load_rom_data(const char* path, const uint8_t* data, size_t size)
{
    const struct LoadRomConfig config =
    {
        .path = path,
        .data = data,
        .size = size,
        .type = LoadRomType_MEM
    };

    return loadrom(&config);
}

bool mgb_save_save_file(const char* path)
{
    if (!mgb_has_rom())
    {
        return false;
    }

    if (GB_has_save(mgb.gb))
    {
        struct SafeString ss = {0};

        if (path)
        {
            strncpy(ss.str, path, sizeof(ss.str) - 1);
        }
        else
        {
            ss = util_create_save_path(mgb.save_folder, mgb.rom_path);
        }

        if (ss_valid(&ss))
        {
            IFile_t* file = icfile_open(ss.str, IFileMode_WRITE, 0);

            if (file)
            {
                const size_t save_size = GB_calculate_savedata_size(mgb.gb);
                const bool result = ifile_write(file, mgb.sram_data, save_size);
                ifile_close(file);

                if (result)
                {
                    mgb_log("[MGB] saved game: %s size: %zu\n", ss.str, save_size);
                    if (mgb.on_file_cb)
                    {
                        mgb.on_file_cb(ss.str, CallbackType_SAVE_SAVE, true);
                    }
                    return true;
                }
                else
                {
                    mgb_log_err("[MGB] failed to write save: %s\n", ss.str);
                    goto fail;
                }
            }
            else
            {
                mgb_log_err("[MGB] failed to open savefile for writing: %s\n", ss.str);
                goto fail;
            }
        }
        else
        {
            mgb_log_err("[MGB] invalid save!\n");
        }

    fail:
        if (mgb.on_file_cb)
        {
            mgb.on_file_cb(ss.str, CallbackType_SAVE_SAVE, false);
        }
        return false;
    }

    return true;
}

bool mgb_save_state_file(const char* path)
{
    IFile_t* file = NULL;
    struct SafeString ss = {0};

    if (path)
    {
        strncpy(ss.str, path, sizeof(ss.str) - 1);
    }
    else
    {
        ss = util_create_state_path(mgb.state_folder, mgb.rom_path);
    }

    if (!ss_valid(&ss))
    {
        goto fail;
    }

    file = igzip_open(ss.str, IFileMode_WRITE, 0);
    if (!file)
    {
        mgb_log_err("[MGB] failed to open\n");
        goto fail;
    }

    if (!GB_savestate(mgb.gb, &mgb.state))
    {
        mgb_log_err("[MGB] failed to state\n");
        goto fail;
    }

    if (!ifile_write(file, &mgb.state, sizeof(mgb.state)))
    {
        mgb_log_err("[MGB] failed to write\n");
        goto fail;
    }

    ifile_close(file);
    mgb_log("[MGB] saved to save state file: %s\n", ss.str);

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(ss.str, CallbackType_SAVE_STATE, true);
    }

    return true;

fail:
    if (file)
    {
        ifile_close(file);
    }

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(ss.str, CallbackType_SAVE_STATE, false);
    }

    mgb_log_err("[MGB] failed to save state file: %s\n", ss.str);

    return false;
}

bool mgb_load_state_file(const char* path)
{
    IFile_t* file = NULL;
    struct SafeString ss = {0};

    if (path)
    {
        strncpy(ss.str, path, sizeof(ss.str) - 1);
    }
    else
    {
        ss = util_create_state_path(mgb.state_folder, mgb.rom_path);
    }

    if (!ss_valid(&ss))
    {
        goto fail;
    }

    mgb_log("[MGB] trying to load state from: %s\n", ss.str);

    file = igzip_open(ss.str, IFileMode_READ, 0);
    if (!file)
    {
        mgb_log_err("[MGB] failed to open file: %s\n", ss.str);
        goto fail;
    }

    // todo: error check this
    if (!ifile_read(file, &mgb.state, sizeof(mgb.state)))
    {
        mgb_log_err("[MGB] failed to read file: %s\n", ss.str);
        goto fail;
    }

    if (!GB_loadstate(mgb.gb, &mgb.state))
    {
        mgb_log_err("[MGB] GB failed to loadstate: %s\n", ss.str);
        goto fail;
    }

    ifile_close(file);

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(ss.str, CallbackType_LOAD_STATE, true);
    }

    return true;

fail:
    if (file)
    {
        ifile_close(file);
    }

    if (mgb.on_file_cb)
    {
        mgb.on_file_cb(ss.str, CallbackType_LOAD_STATE, false);
    }

    mgb_log_err("[MGB] failed to load state from: %s\n", ss.str);

    return false;
}

bool mgb_init(struct GB_Core* gb)
{
    memset(&mgb, 0, sizeof(struct mgb));
    mgb.gb = gb;
    return true;
}

void mgb_exit(void)
{
    #if !defined(ANDROID) && !defined(EMSCRIPTEN)
    if (mgb.gb)
    {
        free_game();
    }
    #endif

    if (mgb.save_folder)
    {
        SDL_free(mgb.save_folder);
        mgb.save_folder = NULL;
    }
    if (mgb.rtc_folder)
    {
        SDL_free(mgb.rtc_folder);
        mgb.rtc_folder = NULL;
    }
    if (mgb.state_folder)
    {
        SDL_free(mgb.state_folder);
        mgb.state_folder = NULL;
    }
}

bool mgb_has_rom()
{
    return mgb.rom_loaded;
}

void mgb_set_save_folder(const char* path)
{
    #ifdef HAS_SDL2
    mgb.save_folder = SDL_strdup(path);
    SDL_CreateDirectory(mgb.save_folder, SDL_DIRECTORY_RWX);
    #endif // HAS_SDL2
}

void mgb_set_rtc_folder(const char* path)
{
    #ifdef HAS_SDL2
    mgb.rtc_folder = SDL_strdup(path);
    SDL_CreateDirectory(mgb.rtc_folder, SDL_DIRECTORY_RWX);
    #endif // HAS_SDL2
}

void mgb_set_state_folder(const char* path)
{
    #ifdef HAS_SDL2
    mgb.state_folder = SDL_strdup(path);
    SDL_CreateDirectory(mgb.state_folder, SDL_DIRECTORY_RWX);
    #endif // HAS_SDL2
}

const char* mgb_get_save_folder(void)
{
    return mgb.save_folder;
}

const char* mgb_get_rtc_folder(void)
{
    return mgb.rtc_folder;
}

const char* mgb_get_state_folder(void)
{
    return mgb.state_folder;
}

void mgb_set_on_file_callback(void (*cb)(const char*, enum CallbackType, bool))
{
    mgb.on_file_cb = cb;
}

#include "rewind.h"
#include "compressors.h"

static struct RewindState rewind_state = {0};
static struct Rewind rewinder = {0};

bool mgb_rewind_init(size_t seconds)
{
    rewind_init(&rewinder, seconds);
    rewind_add_compressor(&rewinder, Zlib, Zlib_size);
    // rewind_add_compressor(&rewinder, Zstd, Zstd_size);
    // rewind_add_compressor(&rewinder, Lz4, Lz4_size);
    return true;
}

void mgb_rewind_close(void)
{
    rewind_close(&rewinder);
}

// save states and stores pixel data for that frame
bool mgb_rewind_push_frame(const void* pixels, size_t size)
{
    if (GB_savestate(mgb.gb, &rewind_state.state))
    {
        memcpy(rewind_state.pixels, pixels, size);
        // printf("size is %zu\n", sizeof(rewind_state));
        if (rewind_push(&rewinder, (const uint8_t*)&rewind_state, sizeof(rewind_state)))
        {
            return true;
        }
    }

    return false;
}

// loads state and loads pixel data for that frame
bool mgb_rewind_pop_frame(void* pixels, size_t size)
{
    if (rewind_pop(&rewinder, (uint8_t*)&rewind_state, sizeof(rewind_state)))
    {
        if (GB_loadstate(mgb.gb, &rewind_state.state))
        {
            memcpy(pixels, rewind_state.pixels, size);
            return true;
        }
    }

    return false;
}
