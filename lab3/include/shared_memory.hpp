#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

class SharedMemory {
public:
    static SharedMemory create(const std::string& name, std::size_t size);
    static SharedMemory open(const std::string& name);

    ~SharedMemory();

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    SharedMemory(SharedMemory&& other) noexcept;
    SharedMemory& operator=(SharedMemory&& other) noexcept;

    void resize(std::size_t size);
    void refresh();

    std::span<std::byte> bytes() const;
    std::size_t size() const { return size_; }
    const std::string& name() const { return name_; }
    bool owner() const { return owner_; }

private:
    SharedMemory(std::string name, int fd, bool owner);

    void map(std::size_t size);
    void unmap();
    void release();

    std::string name_;
    int fd_ = -1;
    void* data_ = nullptr;
    std::size_t size_ = 0;
    bool owner_ = false;
};

void writeMessage(SharedMemory& memory, std::string_view text);

std::string readMessage(SharedMemory& memory);

std::string uniqueSegmentName(std::string_view prefix);
