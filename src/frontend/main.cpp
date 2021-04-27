#include "frontend/mgb.hpp"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "minizip/zip.h"
#include "minizip/ioapi_mem.h"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>


#define SAVE_PATH "/saves"
#define STATE_PATH "/states"
#define CONFIG_PATH "/config"

// we make this global so that C functions can use it
static mgb::App app;

// this is very very hacky, see [download_saves.js]
static void* saves_zip_ptr = NULL;


auto write_save_file(const std::string& path, const char* data, std::size_t len) {
    const auto fullpath = "/saves/" + std::filesystem::path{path}.filename().string();
    printf("full path is: %s size: %lu\n", fullpath.c_str(), len);

    std::ofstream fs{fullpath, std::ios::binary};
    if (fs.good()) {
        fs.write(data, len);
    }
    else {
        printf("failed to open save for writing\n");
        return false; 
    }

    return true;
}

auto zip_saves(const char* folder_path) -> uint32_t {
    zlib_filefunc_def filefunc32{};
    ourmemory_t zipmem{};
    zipmem.grow = 1; // the memory buffer will grow
    fill_memory_filefunc(&filefunc32, &zipmem);

    auto zfile = zipOpen3("__notused__", APPEND_STATUS_CREATE, 0, 0, &filefunc32);
    if (!zfile) {
        std::printf("failed to zip open in memory\n");
        return 0;
    }

    for (auto& entry : std::filesystem::recursive_directory_iterator{folder_path}) {
        if (entry.is_regular_file()) {
            // get the fullpath
            const auto path = entry.path().string();
            std::ifstream fs{path, std::ios::binary};

            if (fs.good()) {

                // read file into buffer
                const auto file_size = entry.file_size();
                std::vector<char> buffer(file_size);
                fs.read(buffer.data(), buffer.size());

                // open the file inside the zip
                if (Z_OK != zipOpenNewFileInZip(zfile,
                    path.c_str(),           // filepath
                    NULL,                   // info, optional
                    NULL, 0,                // extrafield and size, optional
                    NULL, 0,                // extrafield-global and size, optional
                    NULL,                   // comment, optional
                    Z_DEFLATED,             // mode
                    Z_DEFAULT_COMPRESSION   // level
                )) {
                    std::printf("failed to open file in zip: %s\n", path.c_str());
                    continue;
                }

                // write out the entire file
                if (Z_OK != zipWriteInFileInZip(zfile, buffer.data(), buffer.size())) {
                    std::printf("failed to write file in zip: %s\n", path.c_str());
                }

                // don't forget to close when done!
                if (Z_OK != zipCloseFileInZip(zfile)) {
                    std::printf("failed to close file in zip: %s\n", path.c_str());
                }
            }

            else {
                std::printf("failed to open file %s\n", path.c_str());
            }
        }
    }

    zipClose(zfile, NULL);

    if (zipmem.grow && zipmem.base) {
        saves_zip_ptr = zipmem.base;
        return zipmem.size;
    }

    return 0;
}

// these are the emscripten functions that will be called by JS
extern "C" {


EMSCRIPTEN_KEEPALIVE
void em_load_rom_data(const char* name, const uint8_t* data, int len) {
    printf("[EM] loading rom! name: %s len: %d\n", name, len);
    app.LoadRomData(name, data, static_cast<size_t>(len));
}

EMSCRIPTEN_KEEPALIVE
void em_upload_save(const char* name, const char* data, int len) {
    printf("[EM] stroing save! name: %s len: %d\n", name, len);
    write_save_file(name, data, static_cast<size_t>(len));
}

EMSCRIPTEN_KEEPALIVE
uint32_t em_zip_all_saves() {
    return zip_saves("/saves/");
}

EMSCRIPTEN_KEEPALIVE
uint8_t* em_get_zip_saves_data() {
    return (uint8_t*)saves_zip_ptr;
}

EMSCRIPTEN_KEEPALIVE
void em_flush_save() {
    app.FlushSave();
}

EMSCRIPTEN_KEEPALIVE
void em_write_save_cb() {
    EM_ASM(
        FS.syncfs(function (err) {
            if (err) {
                console.log(err);
            }
        });
    );
}

EMSCRIPTEN_KEEPALIVE
void em_loop(void* user) {
    static_cast<mgb::App*>(user)->LoopStep();
}

EMSCRIPTEN_KEEPALIVE
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    EM_ASM(
        FS.mkdir("/saves"); FS.mount(IDBFS, {}, "/saves");
        FS.mkdir("/states"); FS.mount(IDBFS, {}, "/states");
        FS.mkdir("/config"); FS.mount(IDBFS, {}, "/config");

        FS.syncfs(true, function (err) {
            if (err) {
                console.log(err);
            }
        });
    );

    app.SetSavePath("/saves/");
    app.SetRtcPath("/saves/");
    app.SetStatePath("/states/");

    app.SetWriteSaveCB(em_write_save_cb);
    app.SetWriteStateCB(em_write_save_cb);

    // setting 60fps seems to cause many issues in firefox.
    // <= 0 will cause it to *try* to sync to the display refresh rate
    // this is ideal for 60hz monitors, but not for 144hz etc monitors
    // to handle this, the core should only be ran once enough time has passed
    // but should still render at the desired refresh rate.
    // this means, at say 120hz, the same frame will be rendered twice, and
    // the core will run once every ~2 frames (this might vary).
    // this will allow for smooth display, whilst still keeping decent
    // control over the emu core.
    // this same control is needed for 144hz displays when using vsync anyway
    // with SDL or opengl etc...
    const int fps = -1;
    // run forever...
    const int infiniate_loop = 1;

    emscripten_set_main_loop_arg(em_loop, &app, fps, infiniate_loop);

    return 0;
}

} // extern "C"

#else

auto main(int argc, char** argv) -> int {  
    mgb::App app;

    if (argc > 1) {
        if (!app.LoadRom(argv[1])) {
            return -1;
        }
    }

    app.Loop();

    std::printf("exiting...\n");

    return 0;
}

#endif
