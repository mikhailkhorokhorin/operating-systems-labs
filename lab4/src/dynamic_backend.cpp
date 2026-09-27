#include "dynamic_backend.hpp"

#include <stdexcept>
#include <utility>

namespace {

std::string firstLibrary(const std::vector<std::string>& libraries) {
    if (libraries.empty()) {
        throw std::invalid_argument("at least one library is required");
    }
    return libraries.front();
}

}

DynamicBackend::DynamicBackend(std::vector<std::string> libraries)
    : libraries_(std::move(libraries)), loaded_(load(firstLibrary(libraries_))) {
}

DynamicBackend::Loaded DynamicBackend::load(const std::string& path) {
    DynamicLibrary library(path);
    const auto primeCount = library.function<PrimeCountFunc>("primeCount");
    const auto pi = library.function<PiFunc>("pi");
    return {std::move(library), primeCount, pi};
}

int DynamicBackend::primeCount(int a, int b) {
    return loaded_.primeCount(a, b);
}

float DynamicBackend::pi(int k) {
    return loaded_.pi(k);
}

std::string DynamicBackend::toggle() {
    const std::size_t next = (index_ + 1) % libraries_.size();
    loaded_ = load(libraries_[next]);
    index_ = next;
    return "Switched to " + current();
}
