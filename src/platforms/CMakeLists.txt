cmake_minimum_required(VERSION 3.18.0)

if (PLATFORM_LIBRETRO)
    add_subdirectory(libretro)
endif()

if (PLATFORM_SDL2)
    add_subdirectory(sdl2)
endif()

if (PLATFORM_GAMECUBE)
    add_subdirectory(gamecube)
endif()
