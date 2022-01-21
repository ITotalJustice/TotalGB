cmake_minimum_required(VERSION 3.18.0)

add_subdirectory(cheats)
add_subdirectory(patch)

if (GB_ZROM)
    add_subdirectory(zrom)
endif()
