#include "line_reader.hpp"

#include <poll.h>
#include <unistd.h>

#include <array>
#include <utility>

namespace chat {

LineReader::LineReader(int fd) : fd_(fd) {
}

bool LineReader::takeLine(std::string& line) {
    const auto end = buffer_.find('\n');
    if (end == std::string::npos) {
        if (!eof_ || buffer_.empty()) {
            return false;
        }
        line = std::move(buffer_);
        buffer_.clear();
    } else {
        line = buffer_.substr(0, end);
        buffer_.erase(0, end + 1);
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return true;
}

ReadStatus LineReader::read(std::string& line, std::chrono::milliseconds timeout) {
    while (true) {
        if (takeLine(line)) {
            return ReadStatus::Line;
        }
        if (eof_) {
            return ReadStatus::Eof;
        }
        pollfd descriptor{fd_, POLLIN, 0};
        const int ready = poll(&descriptor, 1, static_cast<int>(timeout.count()));
        if (ready <= 0) {
            return ReadStatus::Timeout;
        }
        std::array<char, 1024> chunk{};
        const ssize_t count = ::read(fd_, chunk.data(), chunk.size());
        if (count > 0) {
            buffer_.append(chunk.data(), static_cast<std::size_t>(count));
        } else if (count == 0 || (descriptor.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0) {
            eof_ = true;
        }
    }
}

}
