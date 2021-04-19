#include "mgb.hpp"

#include <cstdio>


auto main(int argc, char** argv) -> int {  
    mgb::App app;

    if (argc > 1) {
        if (!app.LoadRom(argv[1])) {
            return -1;
        }
    }

    app.Loop();

    printf("exiting...\n");

    return 0;
}
