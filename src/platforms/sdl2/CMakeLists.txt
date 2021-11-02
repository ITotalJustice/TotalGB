cmake_minimum_required(VERSION 3.18.0)


if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    add_executable(index platform_sdl2.c)
    target_link_libraries(index LINK_PRIVATE
        TotalGB
        util
    )

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png]")
    set(CMAKE_EXECUTABLE_SUFFIX .html)

    set_target_properties(index PROPERTIES LINK_FLAGS
        "-s USE_SDL=2 \
        -s USE_SDL_IMAGE=2 \
        -s SDL2_IMAGE_FORMATS=[png] \
        -s WASM=1 \
        -lidbfs.js \
        -s EXPORTED_FUNCTIONS=[_main,_malloc,_free] \
        -s --shell-file ${PROJECT_SOURCE_DIR}/src/platforms/emscripten/platform_emscripten.html \
        --preload-file res \
        "
    )

    configure_file("${CMAKE_SOURCE_DIR}/res/icons/favicon.ico" "${CMAKE_CURRENT_BINARY_DIR}/favicon.ico" COPYONLY)
else()
    add_executable(TotalGB_SDL2 platform_sdl2.c)

    find_package(SDL2 CONFIG REQUIRED)
    find_package(sdl2-image CONFIG REQUIRED)

    target_link_libraries(TotalGB_SDL2 LINK_PRIVATE
        TotalGB
        SDL2::SDL2
        SDL2::SDL2main
        SDL2::SDL2_image
        util
    )

    if (VCPKG_TOOLCHAIN)
        message("building against vcpkg!!!")
    else()
        message("not building against vcpkg...")
    endif()

    # todo: only enable this if using vcpkg!
    if (VCPKG_TOOLCHAIN)
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            message("static linking sdl!")
            target_link_libraries(TotalGB_SDL2 LINK_PRIVATE SDL2::SDL2-static)
        endif()
    endif()

    message("binary path is: ${CMAKE_CURRENT_BINARY_DIR}")
endif()

configure_file("${CMAKE_SOURCE_DIR}/res/sprites/a.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/a.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/b.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/b.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/up.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/up.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/down.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/down.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/left.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/left.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/right.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/right.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/start.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/start.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/select.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/select.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/volume.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/volume.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/menu.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/menu.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/title.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/title.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/save.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/save.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/load.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/load.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/back.png" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/back.png" COPYONLY)
