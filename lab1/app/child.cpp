#include <exception>
#include <iostream>

#include "line_sum.hpp"

int main() {
    try {
        sumStream(std::cin, std::cout);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
