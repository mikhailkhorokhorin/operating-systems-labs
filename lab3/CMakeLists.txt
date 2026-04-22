cmake_minimum_required(VERSION 3.10)
project(lab3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(lab3_main main.cpp)
add_executable(lab3_child child.cpp)

target_link_libraries(lab3_main rt)
target_link_libraries(lab3_child rt)
