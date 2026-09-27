#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace chat {

enum class CommandType : std::uint8_t { Empty, Quit, Send, History, Invalid };

struct Command {
    CommandType type;
    std::string target;
    std::string text;
};

inline constexpr std::string_view HELP =
    "commands: <recipient>:<message>, /history <keyword>, quit";

std::string_view trim(std::string_view text);

Command parseCommand(std::string_view line);

}
