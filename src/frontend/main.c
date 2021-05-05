#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "mgb.h"


int main(int argc, char** argv) {
    static struct mgb mgb;

    if (!mgb_init(&mgb)) {
        return -1;
    }

    if (argc > 1) {
        if (!mgb_load_rom_file(&mgb, argv[1])) {
            return -1;
        }
    }

    mgb_loop(&mgb);
    mgb_exit(&mgb);

    printf("done!\n");

    return 0;
}
