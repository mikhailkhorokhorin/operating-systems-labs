#include "chat_client.hpp"

#include <unistd.h>

#include <utility>

#include "client_command.hpp"
#include "message_queue.hpp"
#include "sync.hpp"
#include "user_manager.hpp"

namespace chat {

namespace {

using Clock = std::chrono::steady_clock;

std::optional<LoginReply> takeReply(LoginBoard& board, int requestId) {
    for (auto& reply : board.replies) {
        if (reply.requestId == requestId) {
            const LoginReply result = reply;
            reply = LoginReply{};
            return result;
        }
    }
    return std::nullopt;
}

}

ChatClient::ChatClient(SharedMemory& memory, LogSink sink)
    : memory_(&memory), sink_(std::move(sink)) {
}

ChatClient::~ChatClient() {
    logout();
}

LoginStatus ChatClient::login(std::string_view name, std::chrono::milliseconds timeout) {
    if (slot_ >= 0) {
        return LoginStatus::NameTaken;
    }
    if (!isValidName(name)) {
        return LoginStatus::InvalidName;
    }
    const int requestId = memory_->nextRequestId.fetch_add(1) + 1;
    Message request = makeMessage(MessageKind::Login);
    request.requestId = requestId;
    copyBounded(request.from, name);
    if (!sendToServer(request)) {
        return LoginStatus::NoServer;
    }

    const auto deadline = Clock::now() + timeout;
    std::optional<LoginReply> reply;
    {
        RobustLock lock(memory_->logins.mutex);
        while (!(reply = takeReply(memory_->logins, requestId)).has_value()) {
            const auto now = Clock::now();
            if (!serverAlive(*memory_) || now >= deadline) {
                return LoginStatus::NoServer;
            }
            lock.wait(memory_->logins.changed,
                      std::min<Clock::duration>(deadline - now, POLL_INTERVAL));
        }
    }
    if (reply->status != LoginStatus::Accepted) {
        return reply->status;
    }

    name_ = name;
    slot_ = reply->slot;
    running_ = true;
    serverGone_ = false;
    receiver_ = std::thread([this, &queue = memory_->clientQueues[slot_]] { receive(queue); });
    emit("[system] logged in as " + name_ + ", " + std::string(HELP));
    return LoginStatus::Accepted;
}

bool ChatClient::send(std::string_view to, std::string_view text) {
    if (!connected()) {
        return false;
    }
    Message message = makeMessage(MessageKind::Text);
    copyBounded(message.to, to);
    copyBounded(message.text, text);
    return sendToServer(message);
}

bool ChatClient::requestHistory(std::string_view keyword) {
    if (!connected()) {
        return false;
    }
    Message message = makeMessage(MessageKind::History);
    copyBounded(message.text, keyword);
    return sendToServer(message);
}

bool ChatClient::handleLine(std::string_view line) {
    const auto command = parseCommand(line);
    switch (command.type) {
        case CommandType::Quit:
            return false;
        case CommandType::Empty:
            break;
        case CommandType::Send:
            if (!send(command.target, command.text)) {
                emit("[error] message was not sent, server is not running");
            }
            break;
        case CommandType::History:
            if (!requestHistory(command.text)) {
                emit("[error] history request was not sent, server is not running");
            }
            break;
        case CommandType::Invalid:
            emit("[error] " + command.text);
            break;
    }
    return true;
}

void ChatClient::logout() {
    if (slot_ < 0) {
        return;
    }
    if (!serverGone_ && serverAlive(*memory_)) {
        sendToServer(makeMessage(MessageKind::Logout));
    }
    running_ = false;
    if (receiver_.joinable()) {
        receiver_.join();
    }
    slot_ = -1;
}

bool ChatClient::connected() const {
    return slot_ >= 0 && running_.load() && !serverGone_.load();
}

void ChatClient::receive(MessageQueue& queue) {
    while (running_) {
        const auto message = popWait(queue, POLL_INTERVAL);
        if (!message.has_value()) {
            if (!serverAlive(*memory_)) {
                serverStopped();
            }
            continue;
        }
        switch (message->kind) {
            case MessageKind::Text:
                emit("[message] " + std::string(view(message->from)) + ": " +
                     std::string(view(message->text)));
                break;
            case MessageKind::History:
                emit("[history] " + std::string(view(message->from)) + " -> " +
                     std::string(view(message->to)) + ": " + std::string(view(message->text)));
                break;
            case MessageKind::System:
                emit("[system] " + std::string(view(message->text)));
                break;
            case MessageKind::Shutdown:
                serverStopped();
                break;
            case MessageKind::Login:
            case MessageKind::Logout:
                break;
        }
    }
}

void ChatClient::serverStopped() {
    running_ = false;
    if (!serverGone_.exchange(true)) {
        emit("[system] server stopped");
    }
}

bool ChatClient::sendToServer(const Message& message) {
    while (serverAlive(*memory_)) {
        if (pushWait(memory_->serverQueue, message, POLL_INTERVAL)) {
            return true;
        }
    }
    return false;
}

Message ChatClient::makeMessage(MessageKind kind) const {
    Message message{};
    message.kind = kind;
    message.slot = slot_;
    message.pid = getpid();
    copyBounded(message.from, name_);
    return message;
}

void ChatClient::emit(const std::string& line) {
    const std::scoped_lock lock(sinkMutex_);
    if (sink_) {
        sink_(line);
    }
}

int runClientSession(SharedMemory& memory, LineReader& reader, const LogSink& sink,
                     const std::function<bool()>& stopRequested) {
    std::string line;
    auto status = ReadStatus::Timeout;
    while (status == ReadStatus::Timeout && !stopRequested()) {
        status = reader.read(line, POLL_INTERVAL);
    }
    if (status != ReadStatus::Line) {
        sink("[error] no login given");
        return 1;
    }

    ChatClient client(memory, sink);
    const auto result = client.login(trim(line));
    if (result != LoginStatus::Accepted) {
        sink("[error] login failed: " + std::string(describe(result)));
        return 1;
    }
    while (client.connected() && !stopRequested()) {
        status = reader.read(line, POLL_INTERVAL);
        if (status == ReadStatus::Eof) {
            break;
        }
        if (status == ReadStatus::Line && !client.handleLine(line)) {
            break;
        }
    }
    client.logout();
    return 0;
}

}
