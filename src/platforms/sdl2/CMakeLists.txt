cmake_minimum_required(VERSION 3.13.4)


list(APPEND src
    main.c
    audio.c
    touch.c
    font.c
)

add_static_lib(stb stb)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    add_executable(index ${src})

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2")
    set(CMAKE_EXECUTABLE_SUFFIX .html)

    set_target_properties(index PROPERTIES LINK_FLAGS
        " \
        -s USE_PTHREADS=1 \
        -s USE_SDL=2 \
        -s WASM=1 \
        -lidbfs.js \
        -s EXPORTED_FUNCTIONS=[_main,_malloc,_free] \
        -s --shell-file ${PROJECT_SOURCE_DIR}/src/platforms/emscripten/platform_emscripten.html \
        --preload-file res \
        "
    )

    target_link_libraries(index LINK_PRIVATE
        TotalGB
        mgb
        STB::TRUETYPE
        STB::RECT_PACK
    )

    configure_file("${CMAKE_SOURCE_DIR}/res/icons/favicon.ico" "${CMAKE_CURRENT_BINARY_DIR}/favicon.ico" COPYONLY)
    configure_file("${CMAKE_SOURCE_DIR}/res/netlify.toml" "${CMAKE_CURRENT_BINARY_DIR}/netlify.toml" COPYONLY)
else()
    set(myname "TotalGB_SDL2")
    if (ANDROID)
        set(myname "main")
        add_library(${myname} SHARED ${src})
    else()
        add_executable(${myname} ${src})
        find_package(SDL2 CONFIG REQUIRED)
    endif()

    target_link_libraries(${myname} LINK_PRIVATE
        TotalGB
        SDL2::SDL2
        SDL2::SDL2main
        mgb
        STB::TRUETYPE
        STB::RECT_PACK
    )

    if (NINTENDO_SWITCH)
        set_target_properties(${myname} PROPERTIES LINKER_LANGUAGE CXX)

        nx_generate_nacp(${myname}.nacp
            NAME ${myname}
            AUTHOR TotalJustice
            VERSION 0.0.1
        )

        nx_create_nro(${myname}
            ICON ${CMAKE_SOURCE_DIR}/res/icons/icon.jpg
            NACP ${myname}.nacp
            ROMFS ${CMAKE_SOURCE_DIR}/res/sprites
        )
    endif()

    if (VITA)
        include("${VITASDK}/share/vita.cmake" REQUIRED)

        set(VITA_APP_NAME ${myname})
        set(VITA_TITLEID  "VSDK00017")
        set(VITA_VERSION  "01.00")

        target_link_libraries(${myname} LINK_PRIVATE
            SceDisplay_stub
            SceCtrl_stub
            SceAudio_stub
            SceSysmodule_stub
            SceGxm_stub
            SceCommonDialog_stub
            SceAppMgr_stub
            SceTouch_stub
            SceHid_stub
            SceMotion_stub
            m
        )

        vita_create_self(${myname}.self ${myname})
        vita_create_vpk(${myname}.vpk ${VITA_TITLEID} ${myname}.self
            VERSION ${VITA_VERSION}
            NAME ${VITA_APP_NAME}
            # FILE sce_sys/icon0.png sce_sys/icon0.png
            # FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
            # FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
            # FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
        )
    endif()

    target_add_common_cflags(${myname} PRIVATE)

    if (VCPKG_TOOLCHAIN)
        message("building against vcpkg!!!")

        if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            message("static linking sdl!")
            target_link_libraries(${myname} LINK_PRIVATE SDL2::SDL2-static)
        endif()
    endif()

    message("binary path is: ${CMAKE_CURRENT_BINARY_DIR}")
endif()

configure_file("${CMAKE_SOURCE_DIR}/res/sprites/a.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/a.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/b.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/b.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/up.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/up.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/down.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/down.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/left.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/left.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/right.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/right.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/start.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/start.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/select.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/select.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/volume.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/volume.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/menu.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/menu.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/title.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/title.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/save.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/save.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/load.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/load.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/open.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/open.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/options.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/options.bmp" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/res/sprites/back.bmp" "${CMAKE_CURRENT_BINARY_DIR}/res/sprites/back.bmp" COPYONLY)
