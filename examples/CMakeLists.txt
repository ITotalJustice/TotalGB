cmake_minimum_required(VERSION 3.13.4)

if (GB_EXAMPLE_SDL)
    add_executable(example_sdl example_sdl.c)
    target_link_libraries(example_sdl LINK_PRIVATE TotalGB)

    find_package(SDL2 REQUIRED)
    target_include_directories(example_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(example_sdl PRIVATE ${SDL2_LIBRARIES})
endif()

if (GB_EXAMPLE_MMAP)
    add_executable(example_mmap example_mmap.c)
    target_link_libraries(example_mmap LINK_PRIVATE TotalGB)
endif()
