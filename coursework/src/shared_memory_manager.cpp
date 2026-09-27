#include "shared_memory_manager.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace chat {

namespace {

constexpr std::size_t SEGMENT_SIZE = sizeof(SharedMemory);

[[noreturn]] void throwErrno(int error, const std::string& what) {
    throw std::system_error(error, std::system_category(), what);
}

bool removeStaleSegment(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) {
        return errno == ENOENT;
    }
    bool stale = false;
    struct stat info{};
    if (fstat(fd, &info) == 0 && static_cast<std::size_t>(info.st_size) == SEGMENT_SIZE) {
        void* address = mmap(nullptr, SEGMENT_SIZE, PROT_READ, MAP_SHARED, fd, 0);
        if (address != MAP_FAILED) {
            const auto* memory = static_cast<const SharedMemory*>(address);
            stale = memory->serverPid > 0 && !processAlive(memory->serverPid);
            munmap(address, SEGMENT_SIZE);
        }
    }
    close(fd);
    if (stale) {
        shm_unlink(name.c_str());
    }
    return stale;
}

int openExclusive(const std::string& name) {
    return shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
}

}

SharedMemoryManager::SharedMemoryManager(std::string name, SharedMemory* memory, bool owner)
    : name_(std::move(name)), memory_(memory), owner_(owner) {
}

SharedMemoryManager SharedMemoryManager::create(const std::string& name) {
    int fd = openExclusive(name);
    int openError = errno;
    if (fd < 0 && openError == EEXIST && removeStaleSegment(name)) {
        fd = openExclusive(name);
        openError = errno;
    }
    if (fd < 0) {
        if (openError == EEXIST) {
            throw std::runtime_error("shared memory " + name +
                                     " already exists, another server is running");
        }
        throwErrno(openError, "shm_open " + name);
    }
    if (ftruncate(fd, static_cast<off_t>(SEGMENT_SIZE)) != 0) {
        const int error = errno;
        close(fd);
        shm_unlink(name.c_str());
        throwErrno(error, "ftruncate " + name);
    }
    void* address = mmap(nullptr, SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    const int mapError = errno;
    close(fd);
    if (address == MAP_FAILED) {
        shm_unlink(name.c_str());
        throwErrno(mapError, "mmap " + name);
    }
    auto* memory = new (address) SharedMemory{};
    SharedMemoryManager manager(name, memory, true);
    initializeSharedMemory(*memory);
    return manager;
}

SharedMemoryManager SharedMemoryManager::attach(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDWR | O_CLOEXEC, 0);
    if (fd < 0) {
        if (errno == ENOENT) {
            throw std::runtime_error("chat server is not running (no shared memory " + name + ")");
        }
        throwErrno(errno, "shm_open " + name);
    }
    struct stat info{};
    if (fstat(fd, &info) != 0 || static_cast<std::size_t>(info.st_size) != SEGMENT_SIZE) {
        close(fd);
        throw std::runtime_error("shared memory " + name + " has an unexpected size");
    }
    void* address = mmap(nullptr, SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    const int mapError = errno;
    close(fd);
    if (address == MAP_FAILED) {
        throwErrno(mapError, "mmap " + name);
    }
    SharedMemoryManager manager(name, static_cast<SharedMemory*>(address), false);
    const auto& memory = manager.memory();
    if (memory.magic.load(std::memory_order_acquire) != MAGIC || memory.version != VERSION) {
        throw std::runtime_error("shared memory " + name + " is not initialized by a chat server");
    }
    return manager;
}

SharedMemoryManager::~SharedMemoryManager() {
    release();
}

SharedMemoryManager::SharedMemoryManager(SharedMemoryManager&& other) noexcept
    : name_(std::move(other.name_)),
      memory_(std::exchange(other.memory_, nullptr)),
      owner_(std::exchange(other.owner_, false)) {
}

SharedMemoryManager& SharedMemoryManager::operator=(SharedMemoryManager&& other) noexcept {
    if (this != &other) {
        release();
        name_ = std::move(other.name_);
        memory_ = std::exchange(other.memory_, nullptr);
        owner_ = std::exchange(other.owner_, false);
    }
    return *this;
}

void SharedMemoryManager::release() noexcept {
    if (memory_ != nullptr) {
        if (owner_) {
            memory_->serverRunning.store(0);
            memory_->magic.store(0);
        }
        munmap(memory_, SEGMENT_SIZE);
        memory_ = nullptr;
    }
    if (owner_) {
        shm_unlink(name_.c_str());
        owner_ = false;
    }
}

std::string resolveShmName(std::optional<std::string_view> argument) {
    std::string name;
    const char* environment = std::getenv(SHM_NAME_ENV);
    if (argument.has_value() && !argument->empty()) {
        name = *argument;
    } else if (environment != nullptr && *environment != '\0') {
        name = environment;
    } else {
        name = DEFAULT_SHM_NAME;
    }
    if (name.front() != '/') {
        name.insert(name.begin(), '/');
    }
    return name;
}

}
