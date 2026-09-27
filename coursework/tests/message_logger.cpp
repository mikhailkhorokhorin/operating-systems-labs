#include "message_logger.hpp"

#include <gtest/gtest.h>

#include <string>

#include "test_helpers.hpp"

TEST(MessageLoggerTest, RecordsAndPrintsMessages) {
    chat_test::Transcript transcript;
    chat::MessageLogger logger(transcript.sink());
    logger.record(chat_test::makeText(0, "alice", "bob", "hello"));
    logger.log("[system] note");
    EXPECT_EQ(transcript.lines(),
              (std::vector<std::string>{"[message] alice -> bob: hello", "[system] note"}));
    EXPECT_EQ(logger.historySize(), 1U);
}

TEST(MessageLoggerTest, SearchesSenderRecipientAndText) {
    chat::MessageLogger logger(nullptr);
    logger.log("ignored without sink");
    logger.record(chat_test::makeText(0, "alice", "bob", "hello"));
    logger.record(chat_test::makeText(1, "bob", "carol", "lunch?"));
    logger.record(chat_test::makeText(2, "carol", "alice", "sure"));
    EXPECT_EQ(logger.search("carol", "alice").size(), 1U);
    EXPECT_EQ(logger.search("carol", "lunch").size(), 1U);
    EXPECT_EQ(logger.search("bob", "carol").size(), 1U);
    EXPECT_EQ(logger.search("alice", "zzz").size(), 0U);
    EXPECT_EQ(logger.search("alice", "").size(), 2U);
}

TEST(MessageLoggerTest, SearchSkipsConversationsOfOtherUsers) {
    chat::MessageLogger logger(nullptr);
    logger.record(chat_test::makeText(0, "alice", "bob", "secret"));
    logger.record(chat_test::makeText(1, "bob", "carol", "secret"));
    EXPECT_EQ(logger.search("dave", "secret").size(), 0U);
    EXPECT_EQ(logger.search("dave", "").size(), 0U);
    const auto carol = logger.search("carol", "secret");
    ASSERT_EQ(carol.size(), 1U);
    EXPECT_EQ(chat::view(carol.front().from), "bob");
}

TEST(MessageLoggerTest, HistoryIsBounded) {
    chat::MessageLogger logger(nullptr);
    for (std::size_t i = 0; i < chat::MAX_HISTORY + 10; ++i) {
        logger.record(chat_test::makeText(0, "a", "b", "message " + std::to_string(i)));
    }
    EXPECT_EQ(logger.historySize(), chat::MAX_HISTORY);
    EXPECT_TRUE(logger.search("a", "message 0").empty());
    EXPECT_EQ(logger.search("a", "message " + std::to_string(chat::MAX_HISTORY + 9)).size(), 1U);
}
