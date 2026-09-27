lab_library(lab3_core
    SOURCES
        src/line_sum.cpp
        src/pipeline.cpp
        src/process.cpp
        src/shared_memory.cpp
)

lab_executable(lab3_main
    SOURCES app/main.cpp
    LINK lab3_core
)

lab_executable(lab3_child
    SOURCES app/child.cpp
    LINK lab3_core
)

lab_tests(lab3_tests
    SOURCES
        tests/e2e.cpp
        tests/line_sum.cpp
        tests/pipeline.cpp
        tests/process.cpp
        tests/shared_memory.cpp
    LINK lab3_core
    BINARIES lab3_main lab3_child
)
