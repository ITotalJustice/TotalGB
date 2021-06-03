cmake_minimum_required(VERSION 3.13.4)


project(TotalGB
    VERSION 0.0.1
    DESCRIPTION "Simple, fast, very inaccurate GB / GBC emulator!"
    HOMEPAGE_URL "https://github.com/ITotalJustice/TotalGB"
    LANGUAGES C
)


option(GB_SINGLE_FILE "include all src in single.c" OFF)
option(GB_DEBUG "enable debug" OFF)
option(GB_DEV "enables debug and sanitizers" OFF)

option(GB_GAMESHARK "enables gameshark cheats (uses malloc,free)" OFF)
option(GB_ENABLE_CHEATS "enables all cheats" OFF)

option(GB_EXAMPLE_SDL "build basic sdl2 example" OFF)

option(GB_TEST_SANITY "build test that ensures the lib can be built" OFF)
option(GB_TEST_BENCH "" OFF)
option(GB_TEST_ALL "build all tests" OFF)

if (GB_ENABLE_CHEATS)
    set(GB_GAMESHARK ON)
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
