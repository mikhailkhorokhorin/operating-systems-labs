cmake_minimum_required(VERSION 3.10)
project(lab4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_library(lab4_basic_static STATIC lib/basic.cpp)
add_library(lab4_advanced_static STATIC lib/advanced.cpp)

add_library(lab4_basic SHARED lib/basic.cpp)
add_library(lab4_advanced SHARED lib/advanced.cpp)

set_target_properties(lab4_basic PROPERTIES OUTPUT_NAME "basic")
set_target_properties(lab4_advanced PROPERTIES OUTPUT_NAME "advanced")

add_executable(lab4_static app/static.cpp)
add_executable(lab4_dynamic app/dynamic.cpp)

target_link_libraries(lab4_static lab4_basic_static)
target_link_libraries(lab4_dynamic dl)
