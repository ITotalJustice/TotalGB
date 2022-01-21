cmake_minimum_required(VERSION 3.13.4)

add_subdirectory(cheats)
add_subdirectory(patch)

if (GB_ZROM)
    add_subdirectory(zrom)
endif()
