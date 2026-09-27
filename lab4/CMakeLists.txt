lab_library(lab4_basic_static SOURCES src/basic.cpp)
lab_library(lab4_advanced_static SOURCES src/advanced.cpp)

lab_library(lab4_basic SHARED SOURCES src/basic.cpp OUTPUT_NAME basic)
lab_library(lab4_advanced SHARED SOURCES src/advanced.cpp OUTPUT_NAME advanced)

lab_library(lab4_core
    SOURCES
        src/dynamic_backend.cpp
        src/dynamic_library.cpp
        src/shell.cpp
    LINK ${CMAKE_DL_LIBS}
)

lab_executable(lab4_static
    SOURCES app/static.cpp
    LINK lab4_core lab4_basic_static
)

lab_executable(lab4_dynamic
    SOURCES app/dynamic.cpp
    LINK lab4_core
)
set_target_properties(lab4_dynamic PROPERTIES BUILD_RPATH "$ORIGIN")
add_dependencies(lab4_dynamic lab4_basic lab4_advanced)

lab_tests(lab4_tests
    SOURCES
        tests/contract.cpp
        tests/dynamic_backend.cpp
        tests/e2e.cpp
        tests/shell.cpp
    LINK lab4_core
    BINARIES lab4_static lab4_dynamic lab4_basic lab4_advanced
)
