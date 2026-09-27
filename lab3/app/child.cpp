#include <exception>
#include <iostream>

#include "pipeline.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: lab3_child <segment-name>\n";
        return 2;
    }
    try {
        processSegment(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
