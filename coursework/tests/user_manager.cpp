#include "user_manager.hpp"

#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

#include "message_queue.hpp"
#include "test_helpers.hpp"

TEST(IsValidNameTest, AcceptsPrintableNames) {
    EXPECT_TRUE(chat::isValidName("alice"));
    EXPECT_TRUE(chat::isValidName("bob_2"));
    EXPECT_TRUE(chat::isValidName(std::string(chat::MAX_NAME - 1, 'x')));
}

TEST(IsValidNameTest, RejectsBadNames) {
    EXPECT_FALSE(chat::isValidName(""));
    EXPECT_FALSE(chat::isValidName("two words"));
    EXPECT_FALSE(chat::isValidName("a:b"));
    EXPECT_FALSE(chat::isValidName("tab\t"));
    EXPECT_FALSE(chat::isValidName(std::string(chat::MAX_NAME, 'x')));
}

TEST(UserManagerTest, AssignsDistinctSlots) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    const auto alice = users.login("alice", getpid());
    const auto bob = users.login("bob", getpid());
    ASSERT_EQ(alice.status, chat::LoginStatus::Accepted);
    ASSERT_EQ(bob.status, chat::LoginStatus::Accepted);
    EXPECT_NE(alice.slot, bob.slot);
    EXPECT_EQ(users.findUser("alice"), alice.slot);
    EXPECT_EQ(users.findUser("bob"), bob.slot);
    EXPECT_EQ(users.activeSlots().size(), 2U);
}

TEST(UserManagerTest, RejectsDuplicateAndInvalidNames) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    ASSERT_EQ(users.login("alice", getpid()).status, chat::LoginStatus::Accepted);
    EXPECT_EQ(users.login("alice", getpid()).status, chat::LoginStatus::NameTaken);
    EXPECT_EQ(users.login("", getpid()).status, chat::LoginStatus::InvalidName);
}

TEST(UserManagerTest, RejectsWhenFull) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    for (int i = 0; i < chat::MAX_USERS; ++i) {
        ASSERT_EQ(users.login("user" + std::to_string(i), getpid()).status,
                  chat::LoginStatus::Accepted);
    }
    EXPECT_EQ(users.login("extra", getpid()).status, chat::LoginStatus::ServerFull);
}

TEST(UserManagerTest, EmptyNameNeverMatchesFreeSlot) {
    const chat_test::LocalMemory local;
    const chat::UserManager users(local.get());
    EXPECT_EQ(users.findUser(""), std::nullopt);
    EXPECT_EQ(users.findUser("nobody"), std::nullopt);
}

TEST(UserManagerTest, LogoutRequiresMatchingName) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    const auto alice = users.login("alice", getpid());
    EXPECT_FALSE(users.logout(alice.slot, "bob"));
    EXPECT_FALSE(users.logout(-1, "alice"));
    EXPECT_FALSE(users.logout(chat::MAX_USERS, "alice"));
    EXPECT_TRUE(users.owns(alice.slot, "alice"));
    EXPECT_FALSE(users.owns(alice.slot, "bob"));
    EXPECT_FALSE(users.owns(-1, "alice"));
    EXPECT_TRUE(users.logout(alice.slot, "alice"));
    EXPECT_FALSE(users.logout(alice.slot, "alice"));
    EXPECT_EQ(users.findUser("alice"), std::nullopt);
}

TEST(UserManagerTest, NewLoginStartsWithEmptyQueue) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    const auto alice = users.login("alice", getpid());
    ASSERT_TRUE(chat::tryPush(local.get().clientQueues[alice.slot],
                              chat_test::makeText(0, "x", "alice", "stale")));
    ASSERT_TRUE(users.logout(alice.slot, "alice"));
    const auto bob = users.login("bob", getpid());
    EXPECT_EQ(bob.slot, alice.slot);
    EXPECT_EQ(chat::queueSize(local.get().clientQueues[bob.slot]), 0);
}

TEST(UserManagerTest, ReapsDisconnectedClients) {
    const chat_test::LocalMemory local;
    chat::UserManager users(local.get());
    ASSERT_EQ(users.login("ghost", chat_test::deadPid()).status, chat::LoginStatus::Accepted);
    ASSERT_EQ(users.login("alive", getpid()).status, chat::LoginStatus::Accepted);
    const auto reaped = users.reapDisconnected();
    ASSERT_EQ(reaped.size(), 1U);
    EXPECT_EQ(reaped.front(), "ghost");
    EXPECT_EQ(users.findUser("ghost"), std::nullopt);
    EXPECT_TRUE(users.findUser("alive").has_value());
}
