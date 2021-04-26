#include "frontend/mgb.hpp"

#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>


#define SAVE_PATH "/saves"
#define STATE_PATH "/states"
#define CONFIG_PATH "/config"


// we make this global so that C functions can use it
static mgb::App app;

#include <string>
#include <filesystem>

auto write_save_file(const std::string& path, const uint8_t* data, std::size_t len) {
    const auto fullpath = "/saves/" + std::filesystem::path{path}.filename().string();
    printf("full path is: %s size: %lu\n", fullpath.c_str(), len);

    auto f = fopen(fullpath.c_str(), "wb");
    if (!f) {
        printf("failed to open save for writing\n");
        return false;
    }

    auto r = fwrite(data, len, 1, f);

    printf("fwrite result is %lu\n", r);
    fclose(f);

    return true;
}

// these are the emscripten functions that will be called by JS
extern "C" {


EMSCRIPTEN_KEEPALIVE
void em_load_rom_data(const char* name, const uint8_t* data, int len) {
    printf("[EM] loading rom! name: %s len: %d\n", name, len);
    app.LoadRomData(name, data, static_cast<size_t>(len));
}

EMSCRIPTEN_KEEPALIVE
void em_upload_save(const char* name, const uint8_t* data, int len) {
    printf("[EM] stroing save! name: %s len: %d\n", name, len);
    write_save_file(name, data, static_cast<size_t>(len));
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
                // todo: handle
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
                // todo: handle
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
