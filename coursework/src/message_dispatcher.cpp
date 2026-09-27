#include "message_dispatcher.hpp"

#include "message_queue.hpp"

namespace chat {

MessageDispatcher::MessageDispatcher(SharedMemory& memory, const UserManager& users)
    : memory_(&memory), users_(&users) {
}

DeliveryStatus MessageDispatcher::deliver(const Message& message) {
    const auto slot = users_->findUser(view(message.to));
    if (!slot.has_value()) {
        return DeliveryStatus::UnknownRecipient;
    }
    Message delivered = message;
    delivered.kind = MessageKind::Text;
    delivered.slot = *slot;
    return pushTo(*slot, delivered) ? DeliveryStatus::Delivered : DeliveryStatus::QueueFull;
}

bool MessageDispatcher::pushTo(int slot, const Message& message) {
    if (slot < 0 || slot >= MAX_USERS) {
        return false;
    }
    return tryPush(memory_->clientQueues[slot], message);
}

bool MessageDispatcher::notify(int slot, std::string_view text) {
    Message message{};
    message.kind = MessageKind::System;
    message.slot = slot;
    copyBounded(message.from, "server");
    copyBounded(message.text, text);
    return pushTo(slot, message);
}

void MessageDispatcher::broadcastShutdown() {
    Message message{};
    message.kind = MessageKind::Shutdown;
    copyBounded(message.from, "server");
    for (const int slot : users_->activeSlots()) {
        message.slot = slot;
        pushTo(slot, message);
    }
}

}
