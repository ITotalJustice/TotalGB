cmake_minimum_required(VERSION 3.13.4)

add_library(zrom zrom.c)
target_include_directories(zrom PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(zrom PRIVATE c_std_99)

target_link_libraries(zrom LINK_PRIVATE TotalGB)

# find_package(LZ4 REQUIRED)

# if (LZ4_FOUND)
#     target_include_directories(zrom PRIVATE ${LZ4_INCLUDE_DIRS})
#     target_link_libraries (zrom PRIVATE ${LZ4_LIBRARIES})
# endif(LZ4_FOUND)
