#include "shared_memory.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {

constexpr std::size_t HEADER_SIZE = sizeof(std::uint64_t);

[[noreturn]] void throwErrno(const std::string& what) {
    throw std::system_error(errno, std::system_category(), what);
}

std::size_t currentSize(int fd) {
    struct stat info{};
    if (fstat(fd, &info) != 0) {
        throwErrno("fstat");
    }
    return static_cast<std::size_t>(info.st_size);
}

}

SharedMemory::SharedMemory(std::string name, int fd, bool owner)
    : name_(std::move(name)), fd_(fd), owner_(owner) {
}

SharedMemory SharedMemory::create(const std::string& name, std::size_t size) {
    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (fd < 0) {
        throwErrno("shm_open " + name);
    }
    SharedMemory memory(name, fd, true);
    memory.resize(size);
    return memory;
}

SharedMemory SharedMemory::open(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDWR | O_CLOEXEC, 0);
    if (fd < 0) {
        throwErrno("shm_open " + name);
    }
    SharedMemory memory(name, fd, false);
    memory.refresh();
    return memory;
}

SharedMemory::~SharedMemory() {
    release();
}

SharedMemory::SharedMemory(SharedMemory&& other) noexcept
    : name_(std::move(other.name_)),
      fd_(std::exchange(other.fd_, -1)),
      data_(std::exchange(other.data_, nullptr)),
      size_(std::exchange(other.size_, 0)),
      owner_(std::exchange(other.owner_, false)) {
}

SharedMemory& SharedMemory::operator=(SharedMemory&& other) noexcept {
    if (this != &other) {
        release();
        name_ = std::move(other.name_);
        fd_ = std::exchange(other.fd_, -1);
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
        owner_ = std::exchange(other.owner_, false);
    }
    return *this;
}

void SharedMemory::resize(std::size_t size) {
    if (ftruncate(fd_, static_cast<off_t>(size)) != 0) {
        throwErrno("ftruncate " + name_);
    }
    map(size);
}

void SharedMemory::refresh() {
    map(currentSize(fd_));
}

std::span<std::byte> SharedMemory::bytes() const {
    return {static_cast<std::byte*>(data_), size_};
}

void SharedMemory::map(std::size_t size) {
    unmap();
    if (size == 0) {
        return;
    }
    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (data == MAP_FAILED) {
        throwErrno("mmap " + name_);
    }
    data_ = data;
    size_ = size;
}

void SharedMemory::unmap() {
    if (data_ != nullptr) {
        munmap(data_, size_);
        data_ = nullptr;
        size_ = 0;
    }
}

void SharedMemory::release() {
    unmap();
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    if (owner_) {
        shm_unlink(name_.c_str());
        owner_ = false;
    }
}

void writeMessage(SharedMemory& memory, std::string_view text) {
    memory.resize(HEADER_SIZE + text.size());
    const auto bytes = memory.bytes();
    const std::uint64_t length = text.size();
    std::memcpy(bytes.data(), &length, HEADER_SIZE);
    if (!text.empty()) {
        std::memcpy(bytes.subspan(HEADER_SIZE).data(), text.data(), text.size());
    }
}

std::string readMessage(SharedMemory& memory) {
    memory.refresh();
    const auto bytes = memory.bytes();
    if (bytes.size() < HEADER_SIZE) {
        throw std::runtime_error("shared memory segment has no header");
    }
    std::uint64_t length = 0;
    std::memcpy(&length, bytes.data(), HEADER_SIZE);
    if (length > bytes.size() - HEADER_SIZE) {
        throw std::runtime_error("shared memory segment is truncated");
    }
    const auto payload = bytes.subspan(HEADER_SIZE, length);
    return {reinterpret_cast<const char*>(payload.data()), payload.size()};
}

std::string uniqueSegmentName(std::string_view prefix) {
    static std::atomic<unsigned> counter{0};
    return std::string("/") + std::string(prefix) + "-" + std::to_string(getpid()) + "-" +
           std::to_string(counter.fetch_add(1));
}
