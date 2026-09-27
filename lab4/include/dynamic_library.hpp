#pragma once

#include <string>

std::string resolveLibraryPath(const std::string& name);

class DynamicLibrary {
public:
    explicit DynamicLibrary(std::string path);
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    void* symbol(const std::string& name) const;

    template <typename Function>
    Function function(const std::string& name) const {
        return reinterpret_cast<Function>(symbol(name));
    }

    const std::string& path() const { return path_; }

private:
    void close();

    std::string path_;
    void* handle_ = nullptr;
};
