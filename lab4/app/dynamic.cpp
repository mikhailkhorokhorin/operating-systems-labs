#include <unistd.h>

#include <exception>
#include <iostream>

#include "dynamic_backend.hpp"
#include "shell.hpp"

int main() {
    try {
        DynamicBackend backend({"libbasic.so", "libadvanced.so"});
        std::cout << "Loaded " << backend.current() << '\n';
        return runShell(std::cin, std::cout, std::cerr, backend, isatty(STDIN_FILENO) != 0);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
