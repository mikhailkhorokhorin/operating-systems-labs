#include "message_logger.hpp"

#include <utility>

namespace chat {

MessageLogger::MessageLogger(LogSink sink) : sink_(std::move(sink)) {
}

void MessageLogger::log(const std::string& line) const {
    if (sink_) {
        sink_(line);
    }
}

void MessageLogger::record(const Message& message) {
    log("[message] " + std::string(view(message.from)) + " -> " + std::string(view(message.to)) +
        ": " + std::string(view(message.text)));
    history_.push_back(message);
    if (history_.size() > MAX_HISTORY) {
        history_.pop_front();
    }
}

std::vector<Message> MessageLogger::search(std::string_view participant,
                                           std::string_view keyword) const {
    std::vector<Message> matches;
    for (const auto& message : history_) {
        const auto from = view(message.from);
        const auto to = view(message.to);
        if (from != participant && to != participant) {
            continue;
        }
        if (from.find(keyword) != std::string_view::npos ||
            to.find(keyword) != std::string_view::npos ||
            view(message.text).find(keyword) != std::string_view::npos) {
            matches.push_back(message);
        }
    }
    return matches;
}

}
