cmake_minimum_required(VERSION 3.18.0)

project(stb LANGUAGES C)

add_library(stb_rect_pack
    stb_rect_pack.c
)

add_library(stb_truetype
    stb_truetype.c
)

add_library(STB::TRUETYPE ALIAS stb_rect_pack)
add_library(STB::RECT_PACK ALIAS stb_truetype)

if (NOT WIN32)
    target_link_libraries(stb_truetype PRIVATE m)
endif()

target_include_directories(stb_rect_pack INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stb_truetype INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
