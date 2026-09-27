#include <unistd.h>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "cli.hpp"

int main(int argc, char** argv) {
    try {
        const std::vector<std::string> args(argv + 1, argv + argc);
        return runCli(args, std::cin, std::cout, std::cerr, isatty(STDIN_FILENO) != 0);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
