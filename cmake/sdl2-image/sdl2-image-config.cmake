include(CMakeFindDependencyMacro)
find_dependency(SDL2 CONFIG)
include("${CMAKE_CURRENT_LIST_DIR}/sdl2-image.cmake")
