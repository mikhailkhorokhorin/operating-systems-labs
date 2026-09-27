#include "message_queue.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

#include "test_helpers.hpp"

namespace {

constexpr std::chrono::milliseconds SHORT{10};

}

TEST(MessageQueueTest, KeepsFifoOrderAcrossWrapAround) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < chat::MAX_QUEUE - 5; ++i) {
            ASSERT_TRUE(chat::tryPush(queue, chat_test::makeText(i, "a", "b", std::to_string(i))));
        }
        for (int i = 0; i < chat::MAX_QUEUE - 5; ++i) {
            const auto message = chat::popWait(queue, SHORT);
            ASSERT_TRUE(message.has_value());
            EXPECT_EQ(chat::view(message->text), std::to_string(i));
        }
    }
    EXPECT_EQ(chat::queueSize(queue), 0);
}

TEST(MessageQueueTest, RejectsPushWhenFull) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    const auto message = chat_test::makeText(0, "a", "b", "text");
    for (int i = 0; i < chat::MAX_QUEUE; ++i) {
        ASSERT_TRUE(chat::tryPush(queue, message));
    }
    EXPECT_FALSE(chat::tryPush(queue, message));
    EXPECT_FALSE(chat::pushWait(queue, message, SHORT));
    EXPECT_EQ(chat::queueSize(queue), chat::MAX_QUEUE);
}

TEST(MessageQueueTest, PopTimesOutWhenEmpty) {
    const chat_test::LocalMemory local;
    EXPECT_FALSE(chat::popWait(local.get().serverQueue, SHORT).has_value());
}

TEST(MessageQueueTest, PushWaitsForFreeSpace) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    const auto message = chat_test::makeText(0, "a", "b", "text");
    for (int i = 0; i < chat::MAX_QUEUE; ++i) {
        ASSERT_TRUE(chat::tryPush(queue, message));
    }
    std::thread consumer([&queue] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_TRUE(chat::popWait(queue, std::chrono::seconds(1)).has_value());
    });
    EXPECT_TRUE(chat::pushWait(queue, message, std::chrono::seconds(5)));
    consumer.join();
}

TEST(MessageQueueTest, PopWaitsForProducer) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    std::thread producer([&queue] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_TRUE(chat::tryPush(queue, chat_test::makeText(0, "a", "b", "late")));
    });
    const auto message = chat::popWait(queue, std::chrono::seconds(5));
    producer.join();
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(chat::view(message->text), "late");
}

TEST(MessageQueueTest, ClearEmptiesQueue) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    ASSERT_TRUE(chat::tryPush(queue, chat_test::makeText(0, "a", "b", "x")));
    chat::clearQueue(queue);
    EXPECT_EQ(chat::queueSize(queue), 0);
}

TEST(MessageQueueTest, RepairsCorruptedCounters) {
    const chat_test::LocalMemory local;
    auto& queue = local.get().serverQueue;
    queue.count = chat::MAX_QUEUE + 7;
    queue.head = -3;
    EXPECT_EQ(chat::queueSize(queue), 0);
    EXPECT_TRUE(chat::tryPush(queue, chat_test::makeText(0, "a", "b", "x")));
}
