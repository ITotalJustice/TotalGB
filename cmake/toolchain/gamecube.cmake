# this was taken from mgba, and is licensed under MPL2

include(${CMAKE_CURRENT_LIST_DIR}/../devkitpro.cmake)

set(cross_prefix powerpc-eabi-)
set(arch_flags "-mrvl -mcpu=750 -meabi -mhard-float -g")
set(inc_flags "-I${DEVKITPRO}/libogc/include -I${DEVKITPRO}/portlibs/gamecube/include ${arch_flags}")
set(link_flags "-L${DEVKITPRO}/libogc/lib/cube -L${DEVKITPRO}/portlibs/gamecube/lib/ ${arch_flags}")

set(CMAKE_SYSTEM_PROCESSOR powerpc CACHE INTERNAL "processor")
set(CMAKE_LIBRARY_ARCHITECTURE powerpc-none-eabi CACHE INTERNAL "abi")

set(GAMECUBE ON)
add_definitions(-DGEKKO)

create_devkit(PPC)

set(CMAKE_FIND_ROOT_PATH ${DEVKITPPC}/powerpc-eabi ${DEVKITPRO}/portlibs/ppc)
