#pragma once

#include <filesystem>
#include <string_view>

class FileDescriptor {
public:
    FileDescriptor() = default;
    explicit FileDescriptor(int fd);
    ~FileDescriptor();

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }
    void reset();

private:
    int fd_ = -1;
};

std::filesystem::path executableDirectory();

std::filesystem::path siblingExecutable(std::string_view name);

int exitCodeFromStatus(int status);

int waitForChild(int pid);
