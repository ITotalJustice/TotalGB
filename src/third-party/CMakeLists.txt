cmake_minimum_required(VERSION 3.18.0)

# helper macro that works exactly like find_package()
# except that it'll fallback to the static lib if the shared
# lib is not found!
macro(find_shared_lib package subdir)
    find_package(${package} QUIET)
    if (${package}_FOUND)
        message(STATUS "using shared ${package}")
    elseif (NOT ${package}_CHECKED)
        message(STATUS "using static ${package} from third-party/${subdir}")
        set(${package}_CHECKED ON PARENT_SCOPE)
        add_subdirectory(${CMAKE_SOURCE_DIR}/src/third-party/${subdir} ${CMAKE_BINARY_DIR}/src/third-party/${subdir})
    endif()
endmacro()
