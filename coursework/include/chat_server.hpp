#pragma once

#include <sys/types.h>

#include <functional>

#include "message_dispatcher.hpp"
#include "message_logger.hpp"
#include "shared_memory.hpp"
#include "user_manager.hpp"

namespace chat {

inline constexpr std::size_t HISTORY_REPLY_LIMIT = 20;

class ChatServer {
public:
    ChatServer(SharedMemory& memory, LogSink sink);

    void run(const std::function<bool()>& stopRequested);
    void handle(const Message& message);
    void reap();
    void shutdown();

    const MessageLogger& logger() const { return logger_; }

private:
    void handleLogin(const Message& message);
    void handleLogout(const Message& message);
    void handleText(const Message& message);
    void handleHistory(const Message& message);
    bool postLoginReply(int requestId, pid_t pid, const LoginResult& result);

    SharedMemory* memory_;
    UserManager users_;
    MessageLogger logger_;
    MessageDispatcher dispatcher_;
};

}
