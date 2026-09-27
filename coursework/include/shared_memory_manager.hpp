#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "shared_memory.hpp"

namespace chat {

class SharedMemoryManager {
public:
    static SharedMemoryManager create(const std::string& name);
    static SharedMemoryManager attach(const std::string& name);

    ~SharedMemoryManager();

    SharedMemoryManager(const SharedMemoryManager&) = delete;
    SharedMemoryManager& operator=(const SharedMemoryManager&) = delete;
    SharedMemoryManager(SharedMemoryManager&& other) noexcept;
    SharedMemoryManager& operator=(SharedMemoryManager&& other) noexcept;

    SharedMemory& memory() const { return *memory_; }
    const std::string& name() const { return name_; }
    bool owner() const { return owner_; }

private:
    SharedMemoryManager(std::string name, SharedMemory* memory, bool owner);

    void release() noexcept;

    std::string name_;
    SharedMemory* memory_ = nullptr;
    bool owner_ = false;
};

std::string resolveShmName(std::optional<std::string_view> argument);

}
