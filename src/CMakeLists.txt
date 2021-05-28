cmake_minimum_required(VERSION 3.13.4)


add_library(TotalGB
    gb.c
    cpu.c
    bus.c
    joypad.c
    ppu/ppu.c
    ppu/dmg_renderer.c
    ppu/gbc_renderer.c
    ppu/sgb_renderer.c
    apu/apu.c
    apu/io.c
    apu/square1.c
    apu/square2.c
    apu/wave.c
    apu/noise.c
    mbc/mbc.c
    mbc/mbc_0.c
    mbc/mbc_1.c
    mbc/mbc_2.c
    mbc/mbc_3.c
    mbc/mbc_5.c
    timers.c
    serial.c
    sgb.c
    accessories/printer.c
    tables/palette_table.c
)

target_include_directories(TotalGB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

if (GB_DEBUG)
    target_compile_definitions(TotalGB PRIVATE GB_DEBUG=1)
endif()

# TODO: learn how to have this be set by parent cmakefile
# thats including this cmakelist
target_compile_definitions(TotalGB PRIVATE GB_PIXEL_STRIDE=16)
