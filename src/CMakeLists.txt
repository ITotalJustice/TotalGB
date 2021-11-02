cmake_minimum_required(VERSION 3.18.0)

add_subdirectory(core)
add_subdirectory(platforms)

# this works for now, but will need to be more fancy
# as more ports are made
if (PLATFORM_SDL2)
    add_subdirectory(third-party)
    add_subdirectory(util)
endif()
