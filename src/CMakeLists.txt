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
    apu/ch1.c
    apu/ch2.c
    apu/ch3.c
    apu/ch4.c
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

if (GB_DEV)
    set(GB_DEBUG ON)

    list(APPEND _flags
        -Wall
        -Wextra
        -Werror
        -Wfatal-errors
        -Wpedantic
        -Wshadow
        -Wdouble-promotion
        -Wformat=2
        -Wformat-overflow=2
        -Wformat-truncation=1
        -Wundef
        -Wmissing-include-dirs
        -Wstrict-aliasing=2
        -Walloca
        -Wduplicated-branches
        -Wduplicated-cond
        -Wwrite-strings
        -Wdate-time
        -Wsizeof-pointer-memaccess
        -Wlogical-op
        -Wpacked
        -Wredundant-decls
        -Wnested-externs
        -Winline
        -ftrapv
    )

    list(APPEND _sanitizers
        # -fsanitize=address
        # -fsanitize=leak
        -fsanitize=undefined
        # -fsanitize=pointer-compare
        # -fsanitize=pointer-subtract
        -fstack-protector-all
    )

    # include(CheckCCompilerFlag)

    # TODO: check_c_compiler_flag() seems to not work on all flags
    # for some reason. example -Werror fails, even though its valid for gcc...
    
    foreach (flag IN LISTS _flags)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(TotalGB PRIVATE ${flag})
        # endif()
    endforeach()

    foreach (flag IN LISTS _sanitizers)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(TotalGB PRIVATE ${flag})
            target_link_options(TotalGB INTERFACE ${flag})
        # endif()
    endforeach()

endif()

if (GB_DEBUG)
    target_compile_definitions(TotalGB PRIVATE GB_DEBUG=1)
endif()

if (GB_FAST_RW)
    target_compile_definitions(TotalGB PRIVATE GB_FAST_RW=1)
endif()

# TODO: learn how to have this be set by parent cmakefile
# thats including this cmakelist
target_compile_definitions(TotalGB PRIVATE GB_PIXEL_STRIDE=16)
