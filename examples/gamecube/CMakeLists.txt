cmake_minimum_required(VERSION 3.13.4)

# find_program(ELF2DOL elf2dol)

set(BINARY_NAME "TotalGB")

add_executable(${BINARY_NAME}.elf main.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

list(APPEND OS_LIB
    ogc
    fat
    m
)

set(OS_LIB ${OS_LIB} PARENT_SCOPE)

target_link_libraries(${BINARY_NAME}.elf LINK_PRIVATE TotalGB ${OS_LIB})

# add_custom_target(
#     ${BINARY_NAME}.dol ALL
#     ${ELF2DOL} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.elf ${BINARY_NAME}.dol
#     DEPENDS ${BINARY_NAME}.elf
# )
