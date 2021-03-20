#include "mgb.hpp"

#include <cstdio>

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        printf("missing rom path...\n");
        return -1;
    }

    printf("argc value == %d\n", argc);

    mgb::App app;
    switch (argc) {
        case 2:
            if (app.LoadRom(argv[1])) {
                app.Loop();
            }
            break;

        case 3:
            if (app.LoadRom(argv[1]) && app.LoadRom(argv[2], true)) {
                app.Loop();
            }
            break;
    }

    printf("exiting...\n");
    return 0;
}
