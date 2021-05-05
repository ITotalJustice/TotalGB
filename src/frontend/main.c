#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "mui.h"


int main(int argc, char** argv) {
    static struct mui mui;

    if (!mui_init(&mui)) {
        return -1;
    }

    if (argc > 1) {
        if (!mgb_load_rom_file(&mui.mgb, argv[1])) {
            return -1;
        }
    }

    mui_loop(&mui);
    mui_exit(&mui);

    printf("done!\n");

    return 0;
}
