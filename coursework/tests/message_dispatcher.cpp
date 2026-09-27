#include "message_dispatcher.hpp"

#include <gtest/gtest.h>
#include <unistd.h>

#include <chrono>

#include "message_queue.hpp"
#include "test_helpers.hpp"

namespace {

constexpr std::chrono::milliseconds SHORT{10};

}

TEST(MessageDispatcherTest, DeliversToRecipientQueue) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    chat::MessageDispatcher dispatcher(local.get(), users);
    const auto alice = users.login("alice", getpid());
    const auto bob = users.login("bob", getpid());
    EXPECT_EQ(dispatcher.deliver(chat_test::makeText(alice.slot, "alice", "bob", "hi")),
              chat::DeliveryStatus::Delivered);
    const auto message = chat::popWait(local.get().clientQueues[bob.slot], SHORT);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->kind, chat::MessageKind::Text);
    EXPECT_EQ(chat::view(message->from), "alice");
    EXPECT_EQ(chat::view(message->text), "hi");
}

TEST(MessageDispatcherTest, UnknownOrEmptyRecipientIsRejected) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    chat::MessageDispatcher dispatcher(local.get(), users);
    users.login("alice", getpid());
    EXPECT_EQ(dispatcher.deliver(chat_test::makeText(0, "alice", "nobody", "hi")),
              chat::DeliveryStatus::UnknownRecipient);
    EXPECT_EQ(dispatcher.deliver(chat_test::makeText(0, "alice", "", "hi")),
              chat::DeliveryStatus::UnknownRecipient);
    for (auto& queue : local.get().clientQueues) {
        EXPECT_EQ(chat::queueSize(queue), 0);
    }
}

TEST(MessageDispatcherTest, ReportsFullQueue) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    chat::MessageDispatcher dispatcher(local.get(), users);
    users.login("alice", getpid());
    users.login("bob", getpid());
    const auto message = chat_test::makeText(0, "alice", "bob", "spam");
    for (int i = 0; i < chat::MAX_QUEUE; ++i) {
        ASSERT_EQ(dispatcher.deliver(message), chat::DeliveryStatus::Delivered);
    }
    EXPECT_EQ(dispatcher.deliver(message), chat::DeliveryStatus::QueueFull);
}

TEST(MessageDispatcherTest, NotifiesAndBroadcastsShutdown) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    chat::MessageDispatcher dispatcher(local.get(), users);
    const auto alice = users.login("alice", getpid());
    EXPECT_TRUE(dispatcher.notify(alice.slot, "hello"));
    EXPECT_FALSE(dispatcher.notify(-1, "nobody"));
    dispatcher.broadcastShutdown();
    auto& queue = local.get().clientQueues[alice.slot];
    const auto first = chat::popWait(queue, SHORT);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->kind, chat::MessageKind::System);
    EXPECT_EQ(chat::view(first->text), "hello");
    const auto second = chat::popWait(queue, SHORT);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->kind, chat::MessageKind::Shutdown);
}
