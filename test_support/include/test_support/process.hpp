#pragma once

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace test_support {

struct ProcessResult {
    int exitCode = -1;
    std::string out;
    std::string err;
};

inline std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open " + path.string());
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
    if (!file) {
        throw std::runtime_error("cannot write " + path.string());
    }
}

inline std::vector<std::filesystem::path> inputFiles(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".in") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

class TempDir {
public:
    TempDir() {
        std::string pattern = (std::filesystem::temp_directory_path() / "lab-test-XXXXXX").string();
        if (mkdtemp(pattern.data()) == nullptr) {
            throw std::runtime_error("mkdtemp failed");
        }
        path_ = pattern;
    }

    ~TempDir() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = delete;
    TempDir& operator=(TempDir&&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

inline ProcessResult runProcess(const std::string& program,
                                const std::vector<std::string>& args = {},
                                const std::string& input = "",
                                const std::filesystem::path& workDir = {}) {
    const TempDir dir;
    const auto inPath = dir.path() / "stdin";
    const auto outPath = dir.path() / "stdout";
    const auto errPath = dir.path() / "stderr";
    writeFile(inPath, input);

    std::vector<std::string> argvStorage{program};
    argvStorage.insert(argvStorage.end(), args.begin(), args.end());
    std::vector<char*> argv;
    argv.reserve(argvStorage.size() + 1);
    for (auto& arg : argvStorage) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork failed");
    }
    if (pid == 0) {
        const int in = open(inPath.c_str(), O_RDONLY);
        const int out = open(outPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        const int err = open(errPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (in < 0 || out < 0 || err < 0 || dup2(in, STDIN_FILENO) < 0 ||
            dup2(out, STDOUT_FILENO) < 0 || dup2(err, STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(in);
        close(out);
        close(err);
        if (!workDir.empty() && chdir(workDir.c_str()) != 0) {
            _exit(127);
        }
        execv(program.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid failed");
    }

    ProcessResult result;
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    result.out = readFile(outPath);
    result.err = readFile(errPath);
    return result;
}

}
