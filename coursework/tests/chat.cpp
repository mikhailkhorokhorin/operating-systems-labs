#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "chat_client.hpp"
#include "chat_server.hpp"
#include "message_queue.hpp"
#include "test_helpers.hpp"

namespace {

constexpr std::chrono::milliseconds SHORT{10};

class RunningServer {
public:
    explicit RunningServer(chat::SharedMemory& memory)
        : server_(memory, transcript.sink()),
          thread_([this] { server_.run([this] { return stop_.load(); }); }) {}

    ~RunningServer() { stop(); }

    RunningServer(const RunningServer&) = delete;
    RunningServer& operator=(const RunningServer&) = delete;

    void stop() {
        stop_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    chat_test::Transcript transcript;

private:
    std::atomic<bool> stop_{false};
    chat::ChatServer server_;
    std::thread thread_;
};

std::optional<chat::Message> popServer(chat::SharedMemory& memory) {
    return chat::popWait(memory.serverQueue, SHORT);
}

}

TEST(ChatServerTest, HandlesLoginAndRejectsDuplicates) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript transcript;
    chat::ChatServer server(memory, transcript.sink());

    chat::Message login{};
    login.kind = chat::MessageKind::Login;
    login.requestId = 7;
    login.pid = getpid();
    chat::copyBounded(login.from, "alice");
    server.handle(login);
    login.requestId = 8;
    server.handle(login);

    EXPECT_EQ(memory.logins.replies[0].requestId, 7);
    EXPECT_EQ(memory.logins.replies[0].status, chat::LoginStatus::Accepted);
    EXPECT_EQ(memory.logins.replies[1].requestId, 8);
    EXPECT_EQ(memory.logins.replies[1].status, chat::LoginStatus::NameTaken);
    EXPECT_TRUE(transcript.contains("[user] alice has logged in"));
    EXPECT_TRUE(transcript.contains("login rejected for 'alice': name is already taken"));
}

TEST(ChatServerTest, DropsLoginWhenReplyBoardIsFull) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript transcript;
    chat::ChatServer server(memory, transcript.sink());
    for (auto& reply : memory.logins.replies) {
        reply.requestId = 99;
        reply.pid = getpid();
    }
    chat::Message login{};
    login.kind = chat::MessageKind::Login;
    login.requestId = 1;
    chat::copyBounded(login.from, "alice");
    server.handle(login);
    EXPECT_TRUE(transcript.contains("login reply board is full"));
    login.kind = chat::MessageKind::Logout;
    login.slot = 0;
    server.handle(login);
    EXPECT_TRUE(transcript.contains("logout from unknown user 'alice'"));
}

TEST(ChatServerTest, ReusesLoginRepliesOfDeadClients) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat::ChatServer server(memory, nullptr);
    const pid_t dead = chat_test::deadPid();
    for (auto& reply : memory.logins.replies) {
        reply.requestId = 99;
        reply.pid = dead;
    }
    chat::Message login{};
    login.kind = chat::MessageKind::Login;
    login.requestId = 5;
    login.pid = getpid();
    chat::copyBounded(login.from, "alice");
    server.handle(login);
    EXPECT_EQ(memory.logins.replies[0].requestId, 5);
    EXPECT_EQ(memory.logins.replies[0].status, chat::LoginStatus::Accepted);
    EXPECT_EQ(memory.logins.replies[0].pid, getpid());
}

TEST(ChatServerTest, RejectsSpoofedAndUnexpectedMessages) {
    const chat_test::LocalMemory local;
    chat_test::Transcript transcript;
    chat::ChatServer server(local.get(), transcript.sink());
    server.handle(chat_test::makeText(0, "mallory", "bob", "hi"));
    auto history = chat_test::makeText(0, "mallory", "", "hi");
    history.kind = chat::MessageKind::History;
    server.handle(history);
    auto system = chat_test::makeText(0, "mallory", "", "");
    system.kind = chat::MessageKind::Shutdown;
    server.handle(system);
    EXPECT_TRUE(transcript.contains("rejected message from unknown sender 'mallory'"));
    EXPECT_TRUE(transcript.contains("rejected history request from unknown sender 'mallory'"));
    EXPECT_TRUE(transcript.contains("unexpected message from 'mallory'"));
}

TEST(ChatServerTest, ReportsDeliveryFailuresToSender) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript transcript;
    chat::ChatServer server(memory, transcript.sink());
    chat::UserManager users(memory);
    const auto alice = users.login("alice", getpid());
    const auto bob = users.login("bob", getpid());

    server.handle(chat_test::makeText(alice.slot, "alice", "nobody", "hi"));
    auto notice = chat::popWait(memory.clientQueues[alice.slot], SHORT);
    ASSERT_TRUE(notice.has_value());
    EXPECT_EQ(chat::view(notice->text), "user 'nobody' is not online");

    for (int i = 0; i < chat::MAX_QUEUE; ++i) {
        server.handle(chat_test::makeText(alice.slot, "alice", "bob", "spam"));
    }
    server.handle(chat_test::makeText(alice.slot, "alice", "bob", "one too many"));
    notice = chat::popWait(memory.clientQueues[alice.slot], SHORT);
    ASSERT_TRUE(notice.has_value());
    EXPECT_EQ(chat::view(notice->text), "message to 'bob' was not delivered: queue is full");
    EXPECT_EQ(chat::queueSize(memory.clientQueues[bob.slot]), chat::MAX_QUEUE);
    EXPECT_EQ(server.logger().historySize(), static_cast<std::size_t>(chat::MAX_QUEUE));
}

TEST(ChatServerTest, AnswersHistoryRequests) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat::ChatServer server(memory, nullptr);
    chat::UserManager users(memory);
    const auto alice = users.login("alice", getpid());
    users.login("bob", getpid());
    for (std::size_t i = 0; i < chat::HISTORY_REPLY_LIMIT + 5; ++i) {
        server.handle(chat_test::makeText(alice.slot, "alice", "bob", "note " + std::to_string(i)));
    }
    auto request = chat_test::makeText(alice.slot, "alice", "", "note");
    request.kind = chat::MessageKind::History;
    server.handle(request);
    auto& queue = memory.clientQueues[alice.slot];
    EXPECT_EQ(chat::queueSize(queue), static_cast<int>(chat::HISTORY_REPLY_LIMIT));
    const auto first = chat::popWait(queue, SHORT);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->kind, chat::MessageKind::History);
    EXPECT_EQ(chat::view(first->text), "note 5");
    chat::clearQueue(queue);

    chat::copyBounded(request.text, "nothing");
    server.handle(request);
    const auto none = chat::popWait(queue, SHORT);
    ASSERT_TRUE(none.has_value());
    EXPECT_EQ(chat::view(none->text), "no messages match 'nothing'");
}

TEST(ChatServerTest, HistoryShowsOnlyOwnConversations) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat::ChatServer server(memory, nullptr);
    chat::UserManager users(memory);
    const auto alice = users.login("alice", getpid());
    const auto bob = users.login("bob", getpid());
    const auto carol = users.login("carol", getpid());
    server.handle(chat_test::makeText(alice.slot, "alice", "bob", "secret plan"));
    chat::clearQueue(memory.clientQueues[bob.slot]);
    auto request = chat_test::makeText(carol.slot, "carol", "", "secret");
    request.kind = chat::MessageKind::History;
    server.handle(request);
    const auto reply = chat::popWait(memory.clientQueues[carol.slot], SHORT);
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(reply->kind, chat::MessageKind::System);
    EXPECT_EQ(chat::view(reply->text), "no messages match 'secret'");
}

TEST(ChatServerTest, ReapsDeadClientsAndShutsDown) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript transcript;
    chat::ChatServer server(memory, transcript.sink());
    chat::UserManager users(memory);
    users.login("ghost", chat_test::deadPid());
    const auto alive = users.login("alive", getpid());
    server.reap();
    EXPECT_TRUE(transcript.contains("[user] ghost disconnected"));
    server.shutdown();
    EXPECT_EQ(memory.serverRunning.load(), 0);
    const auto message = chat::popWait(memory.clientQueues[alive.slot], SHORT);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->kind, chat::MessageKind::Shutdown);
}

TEST(ChatClientTest, LoginFailsWithoutServer) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat::ChatClient client(memory, nullptr);
    EXPECT_EQ(client.login("bad name"), chat::LoginStatus::InvalidName);
    EXPECT_EQ(client.login("alice", std::chrono::milliseconds(100)), chat::LoginStatus::NoServer);
    ASSERT_TRUE(popServer(memory).has_value());
    memory.serverRunning.store(0);
    EXPECT_EQ(client.login("alice"), chat::LoginStatus::NoServer);
    EXPECT_FALSE(client.connected());
    EXPECT_FALSE(client.send("bob", "hi"));
    EXPECT_FALSE(client.requestHistory("x"));
}

TEST(ChatClientTest, TwoClientsExchangeMessages) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    RunningServer server(memory);
    chat_test::Transcript aliceOut;
    chat_test::Transcript bobOut;
    chat::ChatClient alice(memory, aliceOut.sink());
    chat::ChatClient bob(memory, bobOut.sink());

    ASSERT_EQ(alice.login("alice"), chat::LoginStatus::Accepted);
    ASSERT_EQ(bob.login("bob"), chat::LoginStatus::Accepted);
    EXPECT_NE(alice.slot(), bob.slot());
    EXPECT_EQ(alice.name(), "alice");
    EXPECT_EQ(alice.login("again"), chat::LoginStatus::NameTaken);

    chat::ChatClient impostor(memory, nullptr);
    EXPECT_EQ(impostor.login("alice"), chat::LoginStatus::NameTaken);

    EXPECT_TRUE(alice.handleLine("bob: hello bob"));
    EXPECT_TRUE(bobOut.waitFor("[message] alice: hello bob"));
    EXPECT_TRUE(bob.handleLine("alice:hi alice"));
    EXPECT_TRUE(aliceOut.waitFor("[message] bob: hi alice"));

    EXPECT_TRUE(alice.handleLine("carol:anyone?"));
    EXPECT_TRUE(aliceOut.waitFor("[system] user 'carol' is not online"));
    EXPECT_TRUE(alice.handleLine("/history hello"));
    EXPECT_TRUE(aliceOut.waitFor("[history] alice -> bob: hello bob"));
    EXPECT_TRUE(alice.handleLine("garbage"));
    EXPECT_TRUE(aliceOut.contains("[error] commands:"));
    EXPECT_TRUE(alice.handleLine(""));
    EXPECT_FALSE(alice.handleLine("quit"));

    alice.logout();
    EXPECT_FALSE(alice.connected());
    EXPECT_TRUE(server.transcript.waitFor("[user] alice has logged out"));

    server.stop();
    EXPECT_TRUE(bobOut.waitFor("[system] server stopped"));
    EXPECT_FALSE(bob.connected());
    EXPECT_TRUE(bob.handleLine("alice:late"));
    EXPECT_TRUE(bobOut.contains("[error] message was not sent"));
    EXPECT_TRUE(bob.handleLine("/history x"));
    EXPECT_TRUE(bobOut.contains("[error] history request was not sent"));
}

TEST(ChatClientTest, ReceiverNoticesServerDisappearing) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript out;
    chat::ChatClient client(memory, out.sink());
    {
        RunningServer server(memory);
        ASSERT_EQ(client.login("carol"), chat::LoginStatus::Accepted);
        server.stop();
        EXPECT_TRUE(out.waitFor("[system] server stopped"));
    }
    client.logout();
    EXPECT_FALSE(client.connected());
}

TEST(ChatClientTest, NoticesCrashedServer) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    chat_test::Transcript out;
    chat::ChatClient client(memory, out.sink());
    RunningServer server(memory);
    ASSERT_EQ(client.login("erin"), chat::LoginStatus::Accepted);
    const pid_t serverPid = memory.serverPid;
    memory.serverPid = chat_test::deadPid();
    EXPECT_TRUE(out.waitFor("[system] server stopped"));
    EXPECT_FALSE(client.connected());
    EXPECT_FALSE(client.send("erin", "hello"));
    chat::ChatClient late(memory, nullptr);
    EXPECT_EQ(late.login("frank", std::chrono::milliseconds(100)), chat::LoginStatus::NoServer);
    memory.serverPid = serverPid;
}

TEST(ClientSessionTest, RunsUntilQuit) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    RunningServer server(memory);
    std::array<int, 2> fds{-1, -1};
    ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0);
    const std::string script = "dave\ndave:note to self\nquit\n";
    ASSERT_EQ(write(fds[1], script.data(), script.size()), static_cast<ssize_t>(script.size()));
    chat::LineReader reader(fds[0]);
    chat_test::Transcript out;
    EXPECT_EQ(chat::runClientSession(memory, reader, out.sink(), [] { return false; }), 0);
    EXPECT_TRUE(out.contains("[system] logged in as dave"));
    EXPECT_TRUE(server.transcript.waitFor("[user] dave has logged out"));
    close(fds[0]);
    close(fds[1]);
}

TEST(ClientSessionTest, FailsWithoutLogin) {
    const chat_test::LocalMemory local;
    std::array<int, 2> fds{-1, -1};
    ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0);
    close(fds[1]);
    chat::LineReader reader(fds[0]);
    chat_test::Transcript out;
    EXPECT_EQ(chat::runClientSession(local.get(), reader, out.sink(), [] { return false; }), 1);
    EXPECT_TRUE(out.contains("[error] no login given"));
    close(fds[0]);
}

TEST(ClientSessionTest, ReportsRejectedLogin) {
    const chat_test::LocalMemory local;
    RunningServer server(local.get());
    std::array<int, 2> fds{-1, -1};
    ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0);
    const std::string script = "bad:name\n";
    ASSERT_EQ(write(fds[1], script.data(), script.size()), static_cast<ssize_t>(script.size()));
    chat::LineReader reader(fds[0]);
    chat_test::Transcript out;
    EXPECT_EQ(chat::runClientSession(local.get(), reader, out.sink(), [] { return false; }), 1);
    EXPECT_TRUE(out.contains("[error] login failed: invalid name"));
    close(fds[0]);
    close(fds[1]);
}
