cmake_minimum_required(VERSION 3.18.0)

if (GB_GAMESHARK)
    message(STATUS "GameShark support enabled!")
    add_subdirectory(gameshark)
endif()

if (GB_CODEBREAKER)
    message(STATUS "CodeBreaker support enabled!")
    add_subdirectory(codebreaker)
endif()

if (GB_GAMEGENIE)
    message(STATUS "GameGenie support enabled!")
    add_subdirectory(gamegenie)
endif()
