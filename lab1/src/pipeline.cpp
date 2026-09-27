#include "pipeline.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <istream>
#include <ostream>
#include <string_view>
#include <system_error>

#include "process.hpp"

namespace {

constexpr std::string_view EXEC_FAILED = "error: cannot execute child process\n";

std::string describeErrno() {
    return std::system_category().message(errno);
}

[[noreturn]] void runChild(char* const* argv, int inputFd, int outputFd) {
    if (dup2(inputFd, STDIN_FILENO) < 0 || dup2(outputFd, STDOUT_FILENO) < 0) {
        _exit(126);
    }
    execv(argv[0], argv);
    [[maybe_unused]] const auto written =
        write(STDERR_FILENO, EXEC_FAILED.data(), EXEC_FAILED.size());
    _exit(127);
}

}

std::optional<std::string> readFileName(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::nullopt;
    }
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return std::nullopt;
    }
    const auto last = line.find_last_not_of(" \t\r");
    return line.substr(first, last - first + 1);
}

int runPipeline(const std::filesystem::path& childPath, const std::filesystem::path& inputPath,
                std::ostream& out, std::ostream& err) {
    const FileDescriptor input(open(inputPath.c_str(), O_RDONLY | O_CLOEXEC));
    if (!input.valid()) {
        err << "error: cannot open " << inputPath.string() << ": " << describeErrno() << '\n';
        return 1;
    }

    std::array<int, 2> fds{-1, -1};
    if (pipe2(fds.data(), O_CLOEXEC) != 0) {
        err << "error: pipe: " << describeErrno() << '\n';
        return 1;
    }
    FileDescriptor readEnd(fds[0]);
    FileDescriptor writeEnd(fds[1]);

    std::string program = childPath.string();
    std::array<char*, 2> argv{program.data(), nullptr};
    out.flush();
    const pid_t pid = fork();
    if (pid < 0) {
        err << "error: fork: " << describeErrno() << '\n';
        return 1;
    }
    if (pid == 0) {
        runChild(argv.data(), input.get(), writeEnd.get());
    }
    writeEnd.reset();

    std::array<char, 4096> buffer{};
    while (true) {
        const ssize_t count = read(readEnd.get(), buffer.data(), buffer.size());
        if (count > 0) {
            out.write(buffer.data(), count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count < 0) {
            err << "error: read: " << describeErrno() << '\n';
        }
        break;
    }
    out.flush();
    readEnd.reset();

    const int code = waitForChild(pid);
    if (code != 0) {
        err << "error: child process failed with exit code " << code << '\n';
        return 1;
    }
    return 0;
}
