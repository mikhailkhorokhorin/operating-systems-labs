#pragma once

#include <chrono>
#include <optional>

#include "shared_memory.hpp"

namespace chat {

void initializeQueue(MessageQueue& queue);

void destroyQueue(MessageQueue& queue);

bool tryPush(MessageQueue& queue, const Message& message);

bool pushWait(MessageQueue& queue, const Message& message, std::chrono::nanoseconds timeout);

std::optional<Message> popWait(MessageQueue& queue, std::chrono::nanoseconds timeout);

void clearQueue(MessageQueue& queue);

int queueSize(MessageQueue& queue);

}
