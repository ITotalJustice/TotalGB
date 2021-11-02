cmake_minimum_required(VERSION 3.18.0)

add_library(util
    filedialog.c
    romloader.c
    util.c

    ifile/ifile.c
    ifile/zip/zip.c
    ifile/mem/mem.c
    ifile/cfile/cfile.c
    ifile/gzip/gzip.c
)

target_include_directories(util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_shared_lib(minizip minizip)
find_shared_lib(unofficial-nativefiledialog nativefiledialog)
find_shared_lib(ZLIB zlib)

target_link_libraries(util LINK_PRIVATE
    minizip::minizip
    ZLIB::ZLIB
)

if (HAS_NFD OR unofficial-nativefiledialog_FOUND)
    target_compile_definitions(util PRIVATE HAS_NFD=1)
    target_link_libraries(util LINK_PRIVATE unofficial::nativefiledialog::nfd)
endif()

if (PLATFORM_SDL2 AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    target_compile_definitions(util PRIVATE HAS_SDL2=1)
    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(util LINK_PRIVATE SDL2::SDL2)
endif()
