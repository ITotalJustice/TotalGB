cmake_minimum_required(VERSION 3.13.4)

add_library(gameshark
    gameshark.c
)

target_include_directories(gameshark PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

if (GB_DEV)
    list(APPEND _flags
        -Wall
        -Wextra
        -Werror
        -Wfatal-errors
        -Wpedantic
        -Wshadow
        -Wdouble-promotion
        -Wformat=2
        -Wformat-overflow=2
        -Wformat-truncation=1
        -Wundef
        -Wmissing-include-dirs
        -Wstrict-aliasing=2
        -Walloca
        -Wduplicated-branches
        -Wduplicated-cond
        -Wwrite-strings
        -Wdate-time
        -Wsizeof-pointer-memaccess
        -Wlogical-op
        -Wpacked
        -Wredundant-decls
        -Wnested-externs
        # -Winline
        -ftrapv
    )

    list(APPEND _sanitizers
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize=pointer-compare
        -fsanitize=pointer-subtract
        -fstack-protector-all
    )

    # include(CheckCCompilerFlag)

    # TODO: check_c_compiler_flag() seems to not work on all flags
    # for some reason. example -Werror fails, even though its valid for gcc...
    
    foreach (flag IN LISTS _flags)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(gameshark PRIVATE ${flag})
        # endif()
    endforeach()

    foreach (flag IN LISTS _sanitizers)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(gameshark PRIVATE ${flag})
            target_link_options(gameshark INTERFACE ${flag})
        # endif()
    endforeach()

endif()

