#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#include "line_reader.hpp"
#include "message_logger.hpp"
#include "shared_memory.hpp"

namespace chat {

class ChatClient {
public:
    ChatClient(SharedMemory& memory, LogSink sink);
    ~ChatClient();

    ChatClient(const ChatClient&) = delete;
    ChatClient& operator=(const ChatClient&) = delete;

    LoginStatus login(std::string_view name, std::chrono::milliseconds timeout = LOGIN_TIMEOUT);
    bool send(std::string_view to, std::string_view text);
    bool requestHistory(std::string_view keyword);
    bool handleLine(std::string_view line);
    void logout();

    bool connected() const;
    int slot() const { return slot_; }
    const std::string& name() const { return name_; }

private:
    void receive(MessageQueue& queue);
    void serverStopped();
    bool sendToServer(const Message& message);
    Message makeMessage(MessageKind kind) const;
    void emit(const std::string& line);

    SharedMemory* memory_;
    LogSink sink_;
    std::mutex sinkMutex_;
    std::string name_;
    int slot_ = -1;
    std::atomic<bool> running_{false};
    std::atomic<bool> serverGone_{false};
    std::thread receiver_;
};

int runClientSession(SharedMemory& memory, LineReader& reader, const LogSink& sink,
                     const std::function<bool()>& stopRequested);

}
