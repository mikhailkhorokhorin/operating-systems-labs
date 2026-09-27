find_package(Threads REQUIRED)

lab_library(lab2_core
    SOURCES
        src/cli.cpp
        src/gauss_jordan.cpp
    LINK Threads::Threads
)

lab_executable(lab2_main
    SOURCES app/main.cpp
    LINK lab2_core
)

lab_tests(lab2_tests
    SOURCES
        tests/cli.cpp
        tests/e2e.cpp
        tests/gauss_jordan.cpp
    LINK lab2_core
    BINARIES lab2_main
    LABEL threads
)
