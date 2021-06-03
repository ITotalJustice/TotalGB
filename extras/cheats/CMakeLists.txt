cmake_minimum_required(VERSION 3.13.4)

if (GB_GAMESHARK)
    message(STATUS "Gameshark support enabled!")
    add_subdirectory(gameshark)
endif()
