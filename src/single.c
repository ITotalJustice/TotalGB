// this file is intended to combine all the source files into 1
// thus allowing the compiler to inline basically everything
#include "types.h"

// GB_SINGLE_FILE
#if GB_SINGLE_FILE
    #include "gb.c"
    #include "cpu.c"
    #include "bus.c"
    #include "joypad.c"
    #include "ppu/ppu.c"
    #include "ppu/dmg_renderer.c"
    #include "ppu/gbc_renderer.c"
    #include "ppu/sgb_renderer.c"
    #include "apu/apu.c"
    #include "apu/io.c"
    #include "apu/ch1.c"
    #include "apu/ch2.c"
    #include "apu/ch3.c"
    #include "apu/ch4.c"
    #include "mbc/mbc.c"
    #include "mbc/mbc_0.c"
    #include "mbc/mbc_1.c"
    #include "mbc/mbc_2.c"
    #include "mbc/mbc_3.c"
    #include "mbc/mbc_5.c"
    #include "timers.c"
    #include "serial.c"
    #include "sgb.c"
    #include "accessories/printer.c"
    #include "tables/palette_table.c"
#endif // GB_SINGLE_FILE
