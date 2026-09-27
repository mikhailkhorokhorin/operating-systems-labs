#include "chat_server.hpp"

#include <chrono>
#include <string>
#include <utility>

#include "message_queue.hpp"
#include "sync.hpp"

namespace chat {

namespace {

std::string quoted(std::string_view text) {
    return "'" + std::string(text) + "'";
}

}

ChatServer::ChatServer(SharedMemory& memory, LogSink sink)
    : memory_(&memory), users_(memory), logger_(std::move(sink)), dispatcher_(memory, users_) {
}

void ChatServer::run(const std::function<bool()>& stopRequested) {
    logger_.log("[system] Server started");
    auto lastReap = std::chrono::steady_clock::now();
    while (!stopRequested()) {
        const auto message = popWait(memory_->serverQueue, POLL_INTERVAL);
        if (message.has_value()) {
            handle(*message);
        }
        const auto now = std::chrono::steady_clock::now();
        if (now - lastReap >= REAP_INTERVAL) {
            lastReap = now;
            reap();
        }
    }
    shutdown();
    logger_.log("[system] Server stopped");
}

void ChatServer::handle(const Message& message) {
    switch (message.kind) {
        case MessageKind::Login:
            handleLogin(message);
            break;
        case MessageKind::Logout:
            handleLogout(message);
            break;
        case MessageKind::Text:
            handleText(message);
            break;
        case MessageKind::History:
            handleHistory(message);
            break;
        case MessageKind::System:
        case MessageKind::Shutdown:
            logger_.log("[error] unexpected message from " + quoted(view(message.from)));
            break;
    }
}

void ChatServer::reap() {
    for (const auto& name : users_.reapDisconnected()) {
        logger_.log("[user] " + name + " disconnected");
    }
}

void ChatServer::shutdown() {
    memory_->serverRunning.store(0);
    dispatcher_.broadcastShutdown();
    const RobustLock lock(memory_->logins.mutex);
    pthread_cond_broadcast(&memory_->logins.changed);
}

void ChatServer::handleLogin(const Message& message) {
    const auto name = view(message.from);
    const auto result = users_.login(name, message.pid);
    if (!postLoginReply(message.requestId, message.pid, result)) {
        if (result.status == LoginStatus::Accepted) {
            users_.logout(result.slot, name);
        }
        logger_.log("[error] login reply board is full, dropped login of " + quoted(name));
        return;
    }
    if (result.status == LoginStatus::Accepted) {
        logger_.log("[user] " + std::string(name) + " has logged in");
    } else {
        logger_.log("[error] login rejected for " + quoted(name) + ": " +
                    std::string(describe(result.status)));
    }
}

void ChatServer::handleLogout(const Message& message) {
    const auto name = view(message.from);
    if (users_.logout(message.slot, name)) {
        logger_.log("[user] " + std::string(name) + " has logged out");
    } else {
        logger_.log("[error] logout from unknown user " + quoted(name));
    }
}

void ChatServer::handleText(const Message& message) {
    const auto from = view(message.from);
    const auto to = view(message.to);
    if (!users_.owns(message.slot, from)) {
        logger_.log("[error] rejected message from unknown sender " + quoted(from));
        return;
    }
    switch (dispatcher_.deliver(message)) {
        case DeliveryStatus::Delivered:
            logger_.record(message);
            break;
        case DeliveryStatus::UnknownRecipient:
            dispatcher_.notify(message.slot, "user " + quoted(to) + " is not online");
            logger_.log("[error] message from " + std::string(from) + " to " + quoted(to) +
                        " was not delivered: unknown recipient");
            break;
        case DeliveryStatus::QueueFull:
            dispatcher_.notify(message.slot,
                               "message to " + quoted(to) + " was not delivered: queue is full");
            logger_.log("[error] message from " + std::string(from) + " to " + quoted(to) +
                        " was not delivered: queue is full");
            break;
    }
}

void ChatServer::handleHistory(const Message& message) {
    const auto from = view(message.from);
    if (!users_.owns(message.slot, from)) {
        logger_.log("[error] rejected history request from unknown sender " + quoted(from));
        return;
    }
    const auto keyword = view(message.text);
    const auto matches = logger_.search(from, keyword);
    if (matches.empty()) {
        dispatcher_.notify(message.slot, "no messages match " + quoted(keyword));
        return;
    }
    const std::size_t first =
        matches.size() > HISTORY_REPLY_LIMIT ? matches.size() - HISTORY_REPLY_LIMIT : 0;
    for (std::size_t index = first; index < matches.size(); ++index) {
        Message entry = matches[index];
        entry.kind = MessageKind::History;
        entry.slot = message.slot;
        if (!dispatcher_.pushTo(message.slot, entry)) {
            break;
        }
    }
}

bool ChatServer::postLoginReply(int requestId, pid_t pid, const LoginResult& result) {
    const RobustLock lock(memory_->logins.mutex);
    for (auto& reply : memory_->logins.replies) {
        if (reply.requestId == 0 || !processAlive(reply.pid)) {
            reply = LoginReply{requestId, result.slot, result.status, pid};
            pthread_cond_broadcast(&memory_->logins.changed);
            return true;
        }
    }
    return false;
}

}
