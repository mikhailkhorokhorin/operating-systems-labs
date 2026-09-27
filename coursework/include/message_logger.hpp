#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "shared_memory.hpp"

namespace chat {

using LogSink = std::function<void(const std::string&)>;

class MessageLogger {
public:
    explicit MessageLogger(LogSink sink);

    void log(const std::string& line) const;
    void record(const Message& message);

    std::vector<Message> search(std::string_view participant, std::string_view keyword) const;
    std::size_t historySize() const { return history_.size(); }

private:
    LogSink sink_;
    std::deque<Message> history_;
};

}
