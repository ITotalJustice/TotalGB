#include "mgb.hpp"

#include <cstdio>

auto main(int argc, char** argv) -> int {  
    if (argc < 2) {
        printf("missing rom path...\n");
        return -1;
    }

    printf("argc value == %d\n", argc);

    mgb::App app;

    if (app.LoadRom(argv[1])) {
        app.Loop();
    }

    printf("exiting...\n");

    return 0;
}
