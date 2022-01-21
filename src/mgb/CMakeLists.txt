cmake_minimum_required(VERSION 3.13.4)

# disable for now as they're unused
set(IPS OFF)
set(UPS OFF)
set(GAMESHARK OFF)
set(CODEBREAKER OFF)
set(GAMEGENIE OFF)

add_subdirectory(cheats)
add_subdirectory(patch)

add_library(mgb
    mgb.c
    filedialog.c
    romloader.c
    util.c
    rewind.c
    compressors.c

    ifile/ifile.c
    ifile/zip/zip.c
    ifile/mem/mem.c
    ifile/cfile/cfile.c
    ifile/gzip/gzip.c
)

target_include_directories(mgb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

option(FORCE_STATIC_LIBS OFF)
option(STATIC_MINIZIP OFF)
option(STATIC_ZLIB OFF)
option(STATIC_NFD OFF)

if (VITA)
    set(STATIC_MINIZIP ON)
endif()

if (FORCE_STATIC_LIBS)
    set(STATIC_MINIZIP ON)
    set(STATIC_ZLIB ON)
    set(STATIC_NFD ON)
endif()

add_static_lib(minizip minizip)
find_shared_lib(unofficial-nativefiledialog nativefiledialog)
find_shared_lib(ZLIB zlib)
# find_shared_lib(zstd zstd)
# find_shared_lib(lz4 zstd)

# find_package(zstd CONFIG REQUIRED)
# find_package(lz4 CONFIG REQUIRED)

target_add_common_cflags(mgb PRIVATE)

target_link_libraries(mgb LINK_PRIVATE
    TotalGB
    minizip::minizip
    ZLIB::ZLIB
)

if (HAS_NFD OR unofficial-nativefiledialog_FOUND)
    target_compile_definitions(mgb PRIVATE HAS_NFD=1)
    target_link_libraries(mgb LINK_PRIVATE unofficial::nativefiledialog::nfd)
endif()

if (PLATFORM_SDL2)
    target_compile_definitions(mgb PRIVATE HAS_SDL2=1)
    target_sources(mgb PRIVATE sdl_helper.c)

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2")
    elseif (ANDROID OR ANDROID_TESTING)
        target_link_libraries(mgb LINK_PRIVATE SDL2::SDL2)
    else ()
        find_package(SDL2 CONFIG REQUIRED)
        target_link_libraries(mgb LINK_PRIVATE SDL2::SDL2)
    endif()
endif()
