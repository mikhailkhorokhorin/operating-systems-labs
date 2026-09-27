#include "process.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>
#include <utility>

FileDescriptor::FileDescriptor(int fd) : fd_(fd) {
}

FileDescriptor::~FileDescriptor() {
    reset();
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(std::exchange(other.fd_, -1)) {
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
    if (this != &other) {
        reset();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

void FileDescriptor::reset() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

std::filesystem::path executableDirectory() {
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
}

std::filesystem::path siblingExecutable(std::string_view name) {
    return executableDirectory() / name;
}

int exitCodeFromStatus(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}

int waitForChild(int pid) {
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            throw std::system_error(errno, std::system_category(), "waitpid");
        }
    }
    return exitCodeFromStatus(status);
}
