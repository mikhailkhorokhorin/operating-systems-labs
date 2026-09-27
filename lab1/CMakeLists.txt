lab_library(lab1_core
    SOURCES
        src/line_sum.cpp
        src/pipeline.cpp
        src/process.cpp
)

lab_executable(lab1_main
    SOURCES app/main.cpp
    LINK lab1_core
)

lab_executable(lab1_child
    SOURCES app/child.cpp
    LINK lab1_core
)

lab_tests(lab1_tests
    SOURCES
        tests/e2e.cpp
        tests/line_sum.cpp
        tests/pipeline.cpp
        tests/process.cpp
    LINK lab1_core
    BINARIES lab1_main lab1_child
)
