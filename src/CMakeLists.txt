cmake_minimum_required(VERSION 3.18.0)

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


list(APPEND gcc_flags
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
    -fstrict-aliasing
    -Wstrict-aliasing=3
    -Wstrict-overflow=5
    -Walloca
    -Wduplicated-branches
    -Wduplicated-cond
    -Wwrite-strings
    -Wdate-time
    -Wlogical-op
    -Wpacked
    -Wredundant-decls
    -Wnested-externs
    -Wcast-qual
    -Wcast-align=strict
    -Wunused-macros
    -Wbad-function-cast
)

list(APPEND clang_flags
    -Wall
    -Wextra
    -Werror
    -Wfatal-errors
    -Wpedantic
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wundef
    -Wmissing-include-dirs
    -Wstrict-aliasing=2
    -Wstrict-overflow=5
    -Walloca
    -Wwrite-strings
    -Wdate-time
    -Wpacked
    -Wredundant-decls
    -Wnested-externs
    -Wcast-qual
    -Wcast-align
    -Wunused-macros
    -Wreserved-id-macro
    -Wbad-function-cast
    -Wbitwise-op-parentheses
    -Wbitfield-enum-conversion
    -Wno-missing-braces # false positive

    # can try enabling this just to see what breaks (still compiles as of clang 11)
    # be sure to enable (well, disable) the below flags else a *lot* of "errors"
    # -Weverything
    # -Wno-conversion
    # -Wno-sign-conversion
    # -Wno-missing-prototypes
    # -Wno-padded
)

list(APPEND tcc_flags
    -Wall
    -Werror
    -Wwrite-strings
    -Wunsupported
)

list(APPEND msvc_flags
    /W4
    /WX
)

if (COMPILER_FEATURES)
    target_compile_features(TotalGB PRIVATE c_std_99)
endif()


include(CheckCCompilerFlag)
include(CheckLinkerFlag)

function(add_compiler_flag flag scope)
    # check_c_compiler_flag() adds a macro to the test.c file such that
    # it'll look like # will be shown as "#define COMPILER_SUPPORTS_-Wdouble-promotion"
    # which is not a valid macro! because of this, we have to remove all invalid characters
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" check_flag ${flag})
    check_c_compiler_flag(${flag} CHECK_FLAG_${check_flag})

    if (CHECK_FLAG_${check_flag})
        target_compile_options(TotalGB ${scope} ${flag})
    endif()
endfunction()

function(add_linker_flag flag scope)
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" check_flag ${flag})
    check_linker_flag("C" ${flag} CHECK_FLAG_${check_flag})

    if (CHECK_FLAG_${check_flag})
        target_compile_options(TotalGB ${scope} ${flag})
        message("has linker flag ${flag}")
    else()
        message("does NOT have linker flag ${flag}")
    endif()
endfunction()

function(add_compiler_flags flag_list scope)
    foreach (flag IN LISTS ${flag_list})
       add_compiler_flag(${flag} ${scope})
    endforeach()
endfunction()

function(add_linker_flags flag_list scope)
    foreach (flag IN LISTS ${flag_list})
       add_linker_flag(${flag} ${scope})
    endforeach()
endfunction()

# enable warning flags for each compiler
# only gcc, clang, tcc and msvc are supported so far
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compiler_flags(clang_flags PRIVATE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compiler_flags(gcc_flags PRIVATE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
    add_compiler_flags(tcc_flags PRIVATE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compiler_flags(msvc_flags PRIVATE)
endif()


# enable sanitizer_flags
if (GB_DEV)
    list(APPEND sanitizer_flags
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize=pointer-compare
        -fsanitize=pointer-subtract
        -fstack-protector-all
    )

    add_compiler_flags(sanitizer_flags PUBLIC)
    add_linker_flags(sanitizer_flags PUBLIC)
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

if (GB_MMPX)
    target_compile_definitions(TotalGB INTERFACE GB_MMPX=1)
    target_link_libraries(TotalGB INTERFACE mmpx)
else()
endif()

if (GB_MARCH_NATIVE)
    add_compiler_flag(-march=native PUBLIC)
endif()
