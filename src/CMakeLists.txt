cmake_minimum_required(VERSION 3.13.4)

if (GB_SINGLE_FILE)
    add_library(TotalGB single.c)

    target_compile_definitions(TotalGB PRIVATE GB_SINGLE_FILE=1)
else()
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

    target_compile_definitions(TotalGB PRIVATE GB_SINGLE_FILE=0)
endif()

target_include_directories(TotalGB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(TotalGB PRIVATE c_std_99)

if (GB_DEV)
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
        # -Winline
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
            target_compile_options(TotalGB PUBLIC ${flag})
        # endif()
    endforeach()

    foreach (flag IN LISTS _sanitizers)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(TotalGB PUBLIC ${flag})
            target_link_options(TotalGB PUBLIC ${flag})
        # endif()
    endforeach()

endif()

if (GB_DEBUG)
    target_compile_definitions(TotalGB PRIVATE GB_DEBUG=1)
    set(GB_ENABLE_FORCE_INLINE OFF)
endif()

if (GB_ENABLE_FORCE_INLINE)
    target_compile_definitions(TotalGB PRIVATE GB_ENABLE_FORCE_INLINE=1)
endif()

if (GB_ENABLE_BUILTIN_PALETTE)
    target_compile_definitions(TotalGB PRIVATE GB_ENABLE_BUILTIN_PALETTE=1)
else()
    target_compile_definitions(TotalGB PRIVATE GB_ENABLE_BUILTIN_PALETTE=1)
endif()

if (GBC_ENABLE)
    target_compile_definitions(TotalGB PRIVATE GBC_ENABLE=1)
else()
    target_compile_definitions(TotalGB PRIVATE GBC_ENABLE=0)
endif()

if (SGB_ENABLE)
    target_compile_definitions(TotalGB PRIVATE SGB_ENABLE=1)
else()
    target_compile_definitions(TotalGB PRIVATE SGB_ENABLE=0)
endif()


if (GB_GAMESHARK)
    target_compile_definitions(TotalGB INTERFACE GB_GAMESHARK=1)
    target_link_libraries(TotalGB INTERFACE gameshark)
else()
    target_compile_definitions(TotalGB INTERFACE GB_GAMESHARK=0)
endif()

if (GB_CODEBREAKER)
    target_compile_definitions(TotalGB INTERFACE GB_CODEBREAKER=1)
    target_link_libraries(TotalGB INTERFACE codebreaker)
else()
    target_compile_definitions(TotalGB INTERFACE GB_CODEBREAKER=0)
endif()

if (GB_GAMEGENIE)
    target_compile_definitions(TotalGB INTERFACE GB_GAMEGENIE=1)
    target_link_libraries(TotalGB INTERFACE gamegenie)
else()
    target_compile_definitions(TotalGB INTERFACE GB_GAMEGENIE=0)
endif()

if (GB_IPS)
    target_compile_definitions(TotalGB INTERFACE GB_IPS=1)
    target_link_libraries(TotalGB INTERFACE ips)
else()
    target_compile_definitions(TotalGB INTERFACE GB_IPS=0)
endif()

if (GB_UPS)
    target_compile_definitions(TotalGB INTERFACE GB_UPS=1)
    target_link_libraries(TotalGB INTERFACE ups)
else()
    target_compile_definitions(TotalGB INTERFACE GB_UPS=0)
endif()

if (GB_BPS)
    target_compile_definitions(TotalGB INTERFACE GB_BPS=1)
    target_link_libraries(TotalGB INTERFACE bps)
else()
    target_compile_definitions(TotalGB INTERFACE GB_BPS=0)
endif()

if (GB_ZROM)
    target_compile_definitions(TotalGB INTERFACE GB_ZROM=1)
    target_link_libraries(TotalGB INTERFACE zrom)
else()
    target_compile_definitions(TotalGB INTERFACE GB_ZROM=0)
endif()
