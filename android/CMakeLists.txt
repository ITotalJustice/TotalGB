cmake_minimum_required(VERSION 3.18.0)

include(FetchContent)

# log the git clone
Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(sdl2
    GIT_REPOSITORY https://github.com/ITotalJustice/SDL.git
    GIT_TAG        android_headphone_hack
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(sdl2)
