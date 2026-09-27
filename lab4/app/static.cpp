#include <unistd.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "contract.hpp"
#include "shell.hpp"

namespace {

class StaticBackend final : public Backend {
public:
    int primeCount(int a, int b) override { return ::primeCount(a, b); }
    float pi(int k) override { return ::pi(k); }
    std::string toggle() override {
        throw std::runtime_error("switching implementations requires lab4_dynamic");
    }
};

}

int main() {
    try {
        StaticBackend backend;
        return runShell(std::cin, std::cout, std::cerr, backend, isatty(STDIN_FILENO) != 0);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
