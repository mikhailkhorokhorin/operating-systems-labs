#include <unistd.h>

#include <exception>
#include <iostream>

#include "pipeline.hpp"
#include "process.hpp"

int main() {
    try {
        if (isatty(STDIN_FILENO) != 0) {
            std::cout << "Enter file name: " << std::flush;
        }
        const auto fileName = readFileName(std::cin);
        if (!fileName) {
            std::cerr << "error: no file name given\n";
            return 1;
        }
        return runPipeline(siblingExecutable("lab3_child"), *fileName, std::cout, std::cerr);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
