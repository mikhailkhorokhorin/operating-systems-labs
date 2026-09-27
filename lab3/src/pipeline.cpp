#include "pipeline.hpp"

#include <unistd.h>

#include <array>
#include <cerrno>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string_view>
#include <system_error>

#include "line_sum.hpp"
#include "process.hpp"
#include "shared_memory.hpp"

namespace {

constexpr std::string_view EXEC_FAILED = "error: cannot execute child process\n";

[[noreturn]] void runChild(char* const* argv) {
    execv(argv[0], argv);
    [[maybe_unused]] const auto written =
        write(STDERR_FILENO, EXEC_FAILED.data(), EXEC_FAILED.size());
    _exit(127);
}

std::optional<std::string> readWholeFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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
    const auto content = readWholeFile(inputPath);
    if (!content) {
        err << "error: cannot open " << inputPath.string() << '\n';
        return 1;
    }

    auto memory = SharedMemory::create(uniqueSegmentName("lab3"), 0);
    writeMessage(memory, *content);

    std::string program = childPath.string();
    std::string segment = memory.name();
    std::array<char*, 3> argv{program.data(), segment.data(), nullptr};
    out.flush();
    const pid_t pid = fork();
    if (pid < 0) {
        err << "error: fork: " << std::system_category().message(errno) << '\n';
        return 1;
    }
    if (pid == 0) {
        runChild(argv.data());
    }

    const int code = waitForChild(pid);
    if (code != 0) {
        err << "error: child process failed with exit code " << code << '\n';
        return 1;
    }
    out << readMessage(memory);
    out.flush();
    return 0;
}

void processSegment(const std::string& segmentName) {
    auto memory = SharedMemory::open(segmentName);
    const std::string result = sumText(readMessage(memory));
    writeMessage(memory, result);
}
