#include "client_command.hpp"

#include <utility>

#include "shared_memory.hpp"
#include "user_manager.hpp"

namespace chat {

namespace {

constexpr std::string_view WHITESPACE = " \t\r\n";
constexpr std::string_view HISTORY_PREFIX = "/history";

Command invalid(std::string reason) {
    return {CommandType::Invalid, "", std::move(reason)};
}

}

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(WHITESPACE);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(WHITESPACE);
    return text.substr(first, last - first + 1);
}

Command parseCommand(std::string_view line) {
    const auto command = trim(line);
    if (command.empty()) {
        return {CommandType::Empty, "", ""};
    }
    if (command == "quit" || command == "/quit") {
        return {CommandType::Quit, "", ""};
    }
    if (command.starts_with(HISTORY_PREFIX) &&
        (command.size() == HISTORY_PREFIX.size() ||
         WHITESPACE.find(command[HISTORY_PREFIX.size()]) != std::string_view::npos)) {
        const auto keyword = trim(command.substr(HISTORY_PREFIX.size()));
        if (keyword.size() >= static_cast<std::size_t>(MAX_TEXT)) {
            return invalid("keyword is too long");
        }
        return {CommandType::History, "", std::string(keyword)};
    }
    const auto colon = command.find(':');
    if (colon == std::string_view::npos) {
        return invalid(std::string(HELP));
    }
    const auto target = trim(command.substr(0, colon));
    const auto text = trim(command.substr(colon + 1));
    if (!isValidName(target)) {
        return invalid("invalid recipient name");
    }
    if (text.empty()) {
        return invalid("message is empty");
    }
    if (text.size() >= static_cast<std::size_t>(MAX_TEXT)) {
        return invalid("message is too long");
    }
    return {CommandType::Send, std::string(target), std::string(text)};
}

}
