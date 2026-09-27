#include "client_command.hpp"

#include <gtest/gtest.h>

#include <string>

#include "shared_memory.hpp"

TEST(TrimTest, RemovesSurroundingWhitespace) {
    EXPECT_EQ(chat::trim("  a b \r\n"), "a b");
    EXPECT_EQ(chat::trim(" \t "), "");
}

TEST(ParseCommandTest, RecognisesQuitAndEmpty) {
    EXPECT_EQ(chat::parseCommand("quit").type, chat::CommandType::Quit);
    EXPECT_EQ(chat::parseCommand(" /quit ").type, chat::CommandType::Quit);
    EXPECT_EQ(chat::parseCommand("   ").type, chat::CommandType::Empty);
}

TEST(ParseCommandTest, ParsesMessages) {
    const auto command = chat::parseCommand("bob: hello there ");
    EXPECT_EQ(command.type, chat::CommandType::Send);
    EXPECT_EQ(command.target, "bob");
    EXPECT_EQ(command.text, "hello there");
    EXPECT_EQ(chat::parseCommand("bob:a:b").text, "a:b");
}

TEST(ParseCommandTest, ParsesHistoryRequests) {
    const auto command = chat::parseCommand("/history lunch");
    EXPECT_EQ(command.type, chat::CommandType::History);
    EXPECT_EQ(command.text, "lunch");
    EXPECT_EQ(chat::parseCommand("/history").text, "");
    EXPECT_EQ(chat::parseCommand("/historyx").type, chat::CommandType::Invalid);
}

TEST(ParseCommandTest, RejectsInvalidInput) {
    EXPECT_EQ(chat::parseCommand("no colon").type, chat::CommandType::Invalid);
    EXPECT_EQ(chat::parseCommand("no colon").text, chat::HELP);
    EXPECT_EQ(chat::parseCommand(":text").text, "invalid recipient name");
    EXPECT_EQ(chat::parseCommand("two words:text").text, "invalid recipient name");
    EXPECT_EQ(chat::parseCommand("bob:   ").text, "message is empty");
    EXPECT_EQ(chat::parseCommand("bob:" + std::string(chat::MAX_TEXT, 'x')).text,
              "message is too long");
    EXPECT_EQ(chat::parseCommand("/history " + std::string(chat::MAX_TEXT, 'x')).text,
              "keyword is too long");
}
