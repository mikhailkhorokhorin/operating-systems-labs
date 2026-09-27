find_package(Threads REQUIRED)

lab_library(coursework_core
    SOURCES
        src/chat_client.cpp
        src/chat_server.cpp
        src/client_command.cpp
        src/line_reader.cpp
        src/message_dispatcher.cpp
        src/message_logger.cpp
        src/message_queue.cpp
        src/shared_memory.cpp
        src/shared_memory_manager.cpp
        src/sync.cpp
        src/user_manager.cpp
    LINK Threads::Threads
)

lab_executable(coursework_server
    SOURCES app/server_main.cpp
    LINK coursework_core
)

lab_executable(coursework_client
    SOURCES app/client_main.cpp
    LINK coursework_core
)

lab_tests(coursework_tests
    SOURCES
        tests/chat.cpp
        tests/client_command.cpp
        tests/e2e.cpp
        tests/line_reader.cpp
        tests/message_dispatcher.cpp
        tests/message_logger.cpp
        tests/message_queue.cpp
        tests/shared_memory.cpp
        tests/shared_memory_manager.cpp
        tests/user_manager.cpp
    LINK coursework_core
    BINARIES coursework_server coursework_client
    LABEL threads
)
