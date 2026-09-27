#pragma once

#include <cstdint>
#include <string_view>

#include "shared_memory.hpp"
#include "user_manager.hpp"

namespace chat {

enum class DeliveryStatus : std::uint8_t { Delivered, UnknownRecipient, QueueFull };

class MessageDispatcher {
public:
    MessageDispatcher(SharedMemory& memory, const UserManager& users);

    DeliveryStatus deliver(const Message& message);
    bool pushTo(int slot, const Message& message);
    bool notify(int slot, std::string_view text);
    void broadcastShutdown();

private:
    SharedMemory* memory_;
    const UserManager* users_;
};

}
