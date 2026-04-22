cmake_minimum_required(VERSION 3.10)
project(coursework)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_library(chat_common
    src/shared_memory_manager.cpp
    src/user_manager.cpp
    src/message_dispatcher.cpp
    src/message_logger.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(chat_common
    Threads::Threads
    rt
)

add_executable(server
    app/server_main.cpp
    src/chat_server.cpp
)

add_executable(client
    app/client_main.cpp
    src/chat_client.cpp
)

target_link_libraries(server
    chat_common
)

target_link_libraries(client
    chat_common
)
