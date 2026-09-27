#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "contract.hpp"
#include "dynamic_library.hpp"
#include "shell.hpp"

class DynamicBackend final : public Backend {
public:
    explicit DynamicBackend(std::vector<std::string> libraries);

    int primeCount(int a, int b) override;
    float pi(int k) override;
    std::string toggle() override;

    const std::string& current() const { return loaded_.library.path(); }

private:
    struct Loaded {
        DynamicLibrary library;
        PrimeCountFunc primeCount;
        PiFunc pi;
    };

    static Loaded load(const std::string& path);

    std::vector<std::string> libraries_;
    std::size_t index_ = 0;
    Loaded loaded_;
};
