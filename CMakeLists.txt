cmake_minimum_required(VERSION 3.25)

get_filename_component(REPO_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${REPO_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)
include(Testing)
include(Lab)

enable_testing()

add_subdirectory(cmake/test_support)

file(GLOB LAB_DIRS LIST_DIRECTORIES true RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT LAB_DIRS COMPARE NATURAL)

foreach(dir IN LISTS LAB_DIRS)
    if(NOT dir MATCHES "^(lab[0-9]+|coursework)$")
        continue()
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        message(FATAL_ERROR "${dir} has no CMakeLists.txt")
    endif()
    add_subdirectory(${dir})
endforeach()
