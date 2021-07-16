cmake_minimum_required(VERSION 3.18.0)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


project(TotalGB
    VERSION 0.0.1
    DESCRIPTION "Simple, fast, very inaccurate GB / GBC emulator!"
    HOMEPAGE_URL "https://github.com/ITotalJustice/TotalGB"
    LANGUAGES C
)


message("COMPILER: using ${CMAKE_C_COMPILER_ID}")

# SEE: https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html#supported-compilers
list(APPEND cmake_supported_compiler_features
    AppleClang
    Clang
    GNU
    MSVC
    SunPro
    Intel
)

if (${CMAKE_C_COMPILER_ID} IN_LIST cmake_supported_compiler_features)
    set(COMPILER_FEATURES ON)
    message("compiler features are enabled for this compiler")
else()
    set(COMPILER_FEATURES OFF)
    message("compiler features are NOT enabled for this compiler")
endif()


option(GB_SINGLE_FILE "include all src in single.c" OFF)
option(GB_DEBUG "enable debug" OFF)
option(GB_DEV "enables debug and sanitizers" OFF)
option(GB_ENABLE_FORCE_INLINE "enables force inline, disabled in debug mode" ON)
option(GB_MARCH_NATIVE "enables -march=native, scope is public" OFF)

option(GBC_ENABLE "build with GBC support" ON)
option(SGB_ENABLE "build with SGB support" OFF)
option(GB_ENABLE_BUILTIN_PALETTE "build with builtin palettes" ON)

option(GB_GAMESHARK "enables gameshark cheats (uses malloc,free)" OFF)
option(GB_CODEBREAKER "enables codebreaker cheats (uses malloc,free)" OFF)
option(GB_GAMEGENIE "enables gamegenie cheats (uses malloc,free)" OFF)
option(GB_ENABLE_CHEATS "enables all cheats" OFF)

option(GB_IPS "enables ips" OFF)
option(GB_UPS "enables ups" OFF)
# option(GB_BPS "enables bps" OFF)
option(GB_ENABLE_PATCHES "enables all patches" OFF)

option(GB_ZROM "" OFF)
option(GB_MMPX "enables mmpx 2x scaling" OFF)

option(GB_EXAMPLE_MMAP "build example that shows how to use mmap" OFF)
option(GB_EXAMPLE_SDL "build basic sdl2 example" OFF)
option(GB_EXAMPLE_ALL "builds all examples" OFF)

option(GB_TEST_SANITY "build test that ensures the lib can be built" OFF)
option(GB_TEST_BENCH "" OFF)
option(GB_TEST_ALL "build all tests" OFF)


if (GB_EXAMPLE_ALL)
    set(GB_EXAMPLE_SDL ON)
    set(GB_EXAMPLE_MMAP ON)
endif()


if (GB_ENABLE_CHEATS)
    set(GB_GAMESHARK ON)
    set(GB_CODEBREAKER ON)
    set(GB_GAMEGENIE ON)
endif()

if (GB_ENABLE_PATCHES)
    set(GB_IPS ON)
    set(GB_UPS ON)
    # set(GB_BPS ON)
endif()

if (GB_DEV)
    set(GB_DEBUG ON)
endif()

if (GB_TEST_ALL)
    set(GB_TEST_SANITY ON)
    set(GB_TEST_BENCH ON)
endif()

add_subdirectory(src)
add_subdirectory(extras)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(libs)
