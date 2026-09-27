#include "dynamic_library.hpp"

#include <dlfcn.h>

#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {

std::string lastError(const std::string& fallback) {
    const char* message = dlerror();
    return message != nullptr ? std::string(message) : fallback;
}

}

std::string resolveLibraryPath(const std::string& name) {
    if (name.find('/') != std::string::npos) {
        return name;
    }
    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if (error) {
        return name;
    }
    const auto candidate = executable.parent_path() / name;
    return std::filesystem::exists(candidate, error) ? candidate.string() : name;
}

DynamicLibrary::DynamicLibrary(std::string path)
    : path_(std::move(path)),
      handle_(dlopen(resolveLibraryPath(path_).c_str(), RTLD_NOW | RTLD_LOCAL)) {
    if (handle_ == nullptr) {
        throw std::runtime_error(lastError("cannot load " + path_));
    }
}

DynamicLibrary::~DynamicLibrary() {
    close();
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : path_(std::move(other.path_)), handle_(std::exchange(other.handle_, nullptr)) {
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
    if (this != &other) {
        close();
        path_ = std::move(other.path_);
        handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
}

void* DynamicLibrary::symbol(const std::string& name) const {
    dlerror();
    void* address = dlsym(handle_, name.c_str());
    if (address == nullptr) {
        throw std::runtime_error(lastError("symbol " + name + " is null in " + path_));
    }
    return address;
}

void DynamicLibrary::close() {
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}
