cmake_minimum_required(VERSION 3.13.4)


project(TotalGB
    VERSION 0.0.1
    DESCRIPTION "Simple, fast, very inaccurate GB / GBC emulator!"
    HOMEPAGE_URL "https://github.com/ITotalJustice/TotalGB"
    LANGUAGES C)


option(GB_FAST_RW "" OFF)
option(GB_DEBUG "enable debug" OFF)
option(GB_DEV "enables debug and sanitizers" OFF)
option(GB_TEST_SANITY "build test that ensures the lib can be built" OFF)
option(GB_TEST_BENCH "" OFF)
option(GB_TEST_SDL "build basic sdl2 test" OFF)
option(GB_TEST_ALL "build all tests" OFF)

if (GB_TEST_ALL)
    set(GB_TEST_SANITY ON)
    set(GB_TEST_BENCH ON)
    set(GB_TEST_SDL ON)
endif()

add_subdirectory(src)
add_subdirectory(tests)
