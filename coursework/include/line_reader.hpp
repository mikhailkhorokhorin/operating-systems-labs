#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace chat {

enum class ReadStatus : std::uint8_t { Line, Timeout, Eof };

class LineReader {
public:
    explicit LineReader(int fd);

    ReadStatus read(std::string& line, std::chrono::milliseconds timeout);

private:
    bool takeLine(std::string& line);

    int fd_;
    std::string buffer_;
    bool eof_ = false;
};

}
