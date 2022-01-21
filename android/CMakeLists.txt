cmake_minimum_required(VERSION 3.18.0)

include(FetchContent)

# log the git clone
Set(FETCHCONTENT_QUIET FALSE)

# add_subdirectory(SDL)

FetchContent_Declare(sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE # only for when pulling from master (main)
    # GIT_TAG        ceb9e9a
    GIT_PROGRESS   TRUE
)

# FetchContent_Declare(sdl2_image
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
#     GIT_TAG        main
#     GIT_SHALLOW    TRUE # only for when pulling from master (main)
#     GIT_PROGRESS   TRUE
# )

# FetchContent_Declare(imgui
#     GIT_REPOSITORY https://github.com/ocornut/imgui.git
#     GIT_TAG        master
#     GIT_SHALLOW    TRUE # only for when pulling from master (main)
#     GIT_PROGRESS   TRUE

#     CONFIGURE_COMMAND ""
#     BUILD_COMMAND ""
# )

# set(LIBSAMPLERATE_EXAMPLES FALSE)
# FetchContent_Declare(samplerate
#     GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
#     GIT_TAG        master
#     GIT_SHALLOW    TRUE # only for when pulling from master (main)
#     GIT_PROGRESS   TRUE
# )

FetchContent_MakeAvailable(
    # samplerate
    sdl2
    # imgui
)

# add_library(imgui
#     ${imgui_SOURCE_DIR}/imgui.cpp
#     ${imgui_SOURCE_DIR}/imgui_draw.cpp
#     ${imgui_SOURCE_DIR}/imgui_tables.cpp
#     ${imgui_SOURCE_DIR}/imgui_widgets.cpp
#     ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp
#     ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer.cpp
# )

# target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# target_link_libraries(imgui SDL2::SDL2)

# todo: try and get this to work with gradle
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDevice.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDevice.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceBLESteamController.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceBLESteamController.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceManager.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceManager.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceUSB.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceUSB.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDL.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/SDL.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLActivity.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLActivity.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLAudioManager.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLAudioManager.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLControllerManager.java DESTINATION ${CMAKE_SOURCE_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLControllerManager.java)

# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDevice.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDevice.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceBLESteamController.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceBLESteamController.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceManager.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceManager.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/HIDDeviceUSB.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/HIDDeviceUSB.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDL.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/SDL.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLActivity.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLActivity.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLAudioManager.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLAudioManager.java)
# file(COPY ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLControllerManager.java DESTINATION ${CMAKE_BINARY_DIR}/src/android/app/src/main/java/org/libsdl/app/SDLControllerManager.java)
