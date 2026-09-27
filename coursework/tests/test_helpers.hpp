#pragma once

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "message_logger.hpp"
#include "shared_memory.hpp"
#include "test_support/process.hpp"

namespace chat_test {

class LocalMemory {
public:
    LocalMemory() : memory_(std::make_unique<chat::SharedMemory>()) {
        chat::initializeSharedMemory(*memory_);
    }

    ~LocalMemory() { chat::destroySharedMemory(*memory_); }

    LocalMemory(const LocalMemory&) = delete;
    LocalMemory& operator=(const LocalMemory&) = delete;

    chat::SharedMemory& get() const { return *memory_; }

private:
    std::unique_ptr<chat::SharedMemory> memory_;
};

class Transcript {
public:
    chat::LogSink sink() {
        return [this](const std::string& line) {
            const std::scoped_lock lock(mutex_);
            lines_.push_back(line);
            changed_.notify_all();
        };
    }

    bool waitFor(std::string_view needle,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, timeout, [&] { return containsLocked(needle); });
    }

    bool contains(std::string_view needle) {
        const std::scoped_lock lock(mutex_);
        return containsLocked(needle);
    }

    std::vector<std::string> lines() {
        const std::scoped_lock lock(mutex_);
        return lines_;
    }

private:
    bool containsLocked(std::string_view needle) const {
        return std::ranges::any_of(lines_, [&](const std::string& line) {
            return line.find(needle) != std::string::npos;
        });
    }

    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::string> lines_;
};

inline chat::Message makeText(int slot, std::string_view from, std::string_view to,
                              std::string_view text) {
    chat::Message message{};
    message.kind = chat::MessageKind::Text;
    message.slot = slot;
    chat::copyBounded(message.from, from);
    chat::copyBounded(message.to, to);
    chat::copyBounded(message.text, text);
    return message;
}

inline pid_t deadPid() {
    const pid_t pid = fork();
    if (pid == 0) {
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return pid;
}

inline std::string uniqueShmName(std::string_view tag) {
    static int counter = 0;
    return "/chat-test-" + std::string(tag) + "-" + std::to_string(getpid()) + "-" +
           std::to_string(++counter);
}

class ChildProcess {
public:
    ChildProcess(const std::string& program, const std::vector<std::string>& args,
                 std::filesystem::path outputPath)
        : outputPath_(std::move(outputPath)) {
        std::array<int, 2> fds{-1, -1};
        if (pipe2(fds.data(), O_CLOEXEC) != 0) {
            throw std::runtime_error("pipe failed");
        }
        std::vector<std::string> storage{program};
        storage.insert(storage.end(), args.begin(), args.end());
        std::vector<char*> argv;
        for (auto& arg : storage) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);
        const std::string output = outputPath_.string();
        pid_ = fork();
        if (pid_ < 0) {
            throw std::runtime_error("fork failed");
        }
        if (pid_ == 0) {
            const int out = open(output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (out < 0 || dup2(fds[0], STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0 ||
                dup2(out, STDERR_FILENO) < 0) {
                _exit(127);
            }
            execv(argv[0], argv.data());
            _exit(127);
        }
        close(fds[0]);
        input_ = fds[1];
    }

    ~ChildProcess() {
        closeInput();
        if (!exited_) {
            kill(pid_, SIGKILL);
            int status = 0;
            waitpid(pid_, &status, 0);
        }
    }

    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    void write(std::string_view text) const {
        if (::write(input_, text.data(), text.size()) != static_cast<ssize_t>(text.size())) {
            throw std::runtime_error("write to child failed");
        }
    }

    void closeInput() {
        if (input_ >= 0) {
            close(input_);
            input_ = -1;
        }
    }

    void signal(int number) const { kill(pid_, number); }

    std::optional<int> wait(std::chrono::milliseconds timeout = std::chrono::seconds(10)) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            int status = 0;
            const pid_t result = waitpid(pid_, &status, WNOHANG);
            if (result == pid_) {
                exited_ = true;
                return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return std::nullopt;
    }

    std::string output() const {
        try {
            return test_support::readFile(outputPath_);
        } catch (const std::runtime_error&) {
            return {};
        }
    }

    bool waitForOutput(std::string_view needle,
                       std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (output().find(needle) != std::string::npos) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

private:
    std::filesystem::path outputPath_;
    pid_t pid_ = -1;
    int input_ = -1;
    bool exited_ = false;
};

}
