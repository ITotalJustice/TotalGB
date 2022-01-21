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


if (COMPILER_FEATURES)
    target_compile_features(TotalGB PRIVATE c_std_99)
endif()

target_add_common_cflags(TotalGB PRIVATE)

# enable sanitizer_flags
if (GB_DEV)
    list(APPEND sanitizer_flags
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize=pointer-compare
        -fsanitize=pointer-subtract
        -fsanitize=thread
        -fstack-protector-all
    )

    target_add_compiler_flags(TotalGB sanitizer_flags PUBLIC)
    target_add_linker_flags(TotalGB sanitizer_flags PUBLIC)
endif()

if (GB_DEBUG)
    target_compile_definitions(TotalGB PRIVATE GB_DEBUG=1)
    set(GB_ENABLE_FORCE_INLINE OFF)
endif()

target_compile_definitions(TotalGB PRIVATE
    GBC_ENABLE=$<BOOL:${GBC_ENABLE}>
    SGB_ENABLE=$<BOOL:${SGB_ENABLE}>
    GB_ENABLE_FORCE_INLINE=$<BOOL:${GB_ENABLE_FORCE_INLINE}>
    GB_ENABLE_BUILTIN_PALETTE=$<BOOL:${GB_ENABLE_BUILTIN_PALETTE}>
)
