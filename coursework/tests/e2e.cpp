#include <fcntl.h>
#include <gtest/gtest.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include <string>

#include "test_helpers.hpp"

namespace {

bool segmentExists(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDONLY, 0);
    if (fd < 0) {
        return false;
    }
    close(fd);
    return true;
}

}

TEST(EndToEndTest, TwoClientsChatAndQuit) {
    const test_support::TempDir dir;
    const auto name = chat_test::uniqueShmName("e2e");
    chat_test::ChildProcess server(COURSEWORK_SERVER_PATH, {name}, dir.path() / "server.log");
    ASSERT_TRUE(server.waitForOutput("[system] Server started")) << server.output();

    chat_test::ChildProcess alice(COURSEWORK_CLIENT_PATH, {name}, dir.path() / "alice.log");
    alice.write("alice\n");
    ASSERT_TRUE(server.waitForOutput("[user] alice has logged in")) << server.output();

    chat_test::ChildProcess bob(COURSEWORK_CLIENT_PATH, {name}, dir.path() / "bob.log");
    bob.write("bob\n");
    ASSERT_TRUE(server.waitForOutput("[user] bob has logged in")) << server.output();

    alice.write("bob:hello bob\n");
    EXPECT_TRUE(bob.waitForOutput("[message] alice: hello bob")) << bob.output();
    bob.write("alice:hi alice\n");
    EXPECT_TRUE(alice.waitForOutput("[message] bob: hi alice")) << alice.output();
    EXPECT_TRUE(server.waitForOutput("[message] alice -> bob: hello bob")) << server.output();

    alice.write("quit\n");
    EXPECT_EQ(alice.wait(), 0);
    EXPECT_TRUE(server.waitForOutput("[user] alice has logged out")) << server.output();

    bob.closeInput();
    EXPECT_EQ(bob.wait(), 0);
    EXPECT_TRUE(server.waitForOutput("[user] bob has logged out")) << server.output();

    server.signal(SIGINT);
    EXPECT_EQ(server.wait(), 0);
    EXPECT_TRUE(server.waitForOutput("[system] Server stopped")) << server.output();
    EXPECT_FALSE(segmentExists(name));
}

TEST(EndToEndTest, SecondServerAndDuplicateNameAreRejected) {
    const test_support::TempDir dir;
    const auto name = chat_test::uniqueShmName("e2e");
    chat_test::ChildProcess server(COURSEWORK_SERVER_PATH, {name}, dir.path() / "server.log");
    ASSERT_TRUE(server.waitForOutput("[system] Server started")) << server.output();

    chat_test::ChildProcess second(COURSEWORK_SERVER_PATH, {name}, dir.path() / "second.log");
    EXPECT_EQ(second.wait(), 1);
    EXPECT_NE(second.output().find("another server is running"), std::string::npos);

    chat_test::ChildProcess alice(COURSEWORK_CLIENT_PATH, {name}, dir.path() / "alice.log");
    alice.write("alice\n");
    ASSERT_TRUE(alice.waitForOutput("logged in as alice")) << alice.output();

    chat_test::ChildProcess copy(COURSEWORK_CLIENT_PATH, {name}, dir.path() / "copy.log");
    copy.write("alice\n");
    EXPECT_EQ(copy.wait(), 1);
    EXPECT_NE(copy.output().find("name is already taken"), std::string::npos);

    alice.signal(SIGINT);
    EXPECT_EQ(alice.wait(), 0);
    EXPECT_TRUE(server.waitForOutput("[user] alice has logged out")) << server.output();

    server.signal(SIGTERM);
    EXPECT_EQ(server.wait(), 0);
}

TEST(EndToEndTest, ClientExitsWhenServerStops) {
    const test_support::TempDir dir;
    const auto name = chat_test::uniqueShmName("e2e");
    chat_test::ChildProcess server(COURSEWORK_SERVER_PATH, {name}, dir.path() / "server.log");
    ASSERT_TRUE(server.waitForOutput("[system] Server started")) << server.output();
    chat_test::ChildProcess client(COURSEWORK_CLIENT_PATH, {name}, dir.path() / "client.log");
    client.write("erin\n");
    ASSERT_TRUE(server.waitForOutput("[user] erin has logged in")) << server.output();

    server.signal(SIGINT);
    EXPECT_EQ(server.wait(), 0);
    EXPECT_EQ(client.wait(), 0);
    EXPECT_NE(client.output().find("[system] server stopped"), std::string::npos);
}

TEST(EndToEndTest, ClientWithoutServerFails) {
    const test_support::TempDir dir;
    chat_test::ChildProcess client(COURSEWORK_CLIENT_PATH, {chat_test::uniqueShmName("none")},
                                   dir.path() / "client.log");
    client.closeInput();
    EXPECT_EQ(client.wait(), 1);
    EXPECT_NE(client.output().find("chat server is not running"), std::string::npos);
}

TEST(EndToEndTest, UsageErrors) {
    const auto server = test_support::runProcess(COURSEWORK_SERVER_PATH, {"a", "b"});
    EXPECT_EQ(server.exitCode, 2);
    const auto client = test_support::runProcess(COURSEWORK_CLIENT_PATH, {"a", "b"});
    EXPECT_EQ(client.exitCode, 2);
}
