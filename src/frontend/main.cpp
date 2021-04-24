#include "frontend/mgb.hpp"

#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


auto main(int argc, char** argv) -> int {  
    mgb::App app;

#ifdef __EMSCRIPTEN__
    // silence warnings
    (void)argc; (void)argv;

    constexpr auto rom_path = "roms/POWA! DEMO.zip";
    if (!app.LoadRom(rom_path)) {
    	std::printf("failed to load rom %s\n", rom_path);
    }

    // helper func that calls LoopStep().
    const auto func = [](void* user) -> void {
    	static_cast<mgb::App*>(user)->LoopStep();
    };

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
    const auto fps = -1;
    // run forever...
    const auto infiniate_loop = 1;

    emscripten_set_main_loop_arg(func, &app, fps, infiniate_loop);

#else // __EMSCRIPTEN__
    if (argc > 1) {
        if (!app.LoadRom(argv[1])) {
            return -1;
        }
    }

    app.Loop();
#endif // __EMSCRIPTEN__

    std::printf("exiting...\n");

    return 0;
}
