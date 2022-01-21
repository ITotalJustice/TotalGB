cmake_minimum_required(VERSION 3.13.4)


# todo: support multiple libretro console lib outputs
add_library(totalgb_libretro platform_libretro.c)

target_add_common_cflags(totalgb_libretro PRIVATE)

set_target_properties(totalgb_libretro PROPERTIES PREFIX "")
target_link_libraries(totalgb_libretro LINK_PUBLIC TotalGB)
