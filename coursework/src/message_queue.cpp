#include "message_queue.hpp"

#include "sync.hpp"

namespace chat {

namespace {

using Clock = std::chrono::steady_clock;

void repairIfNeeded(MessageQueue& queue) {
    if (queue.count < 0 || queue.count > MAX_QUEUE || queue.head < 0 || queue.head >= MAX_QUEUE) {
        queue.head = 0;
        queue.count = 0;
    }
}

bool pushLocked(MessageQueue& queue, const Message& message) {
    if (queue.count >= MAX_QUEUE) {
        return false;
    }
    const int tail = (queue.head + queue.count) % MAX_QUEUE;
    queue.messages[tail] = message;
    ++queue.count;
    pthread_cond_signal(&queue.notEmpty);
    return true;
}

}

void initializeQueue(MessageQueue& queue) {
    initRobustMutex(queue.mutex);
    initSharedCondition(queue.notEmpty);
    initSharedCondition(queue.notFull);
    queue.head = 0;
    queue.count = 0;
}

void destroyQueue(MessageQueue& queue) {
    pthread_cond_destroy(&queue.notFull);
    pthread_cond_destroy(&queue.notEmpty);
    pthread_mutex_destroy(&queue.mutex);
}

bool tryPush(MessageQueue& queue, const Message& message) {
    const RobustLock lock(queue.mutex);
    repairIfNeeded(queue);
    return pushLocked(queue, message);
}

bool pushWait(MessageQueue& queue, const Message& message, std::chrono::nanoseconds timeout) {
    const auto deadline = Clock::now() + timeout;
    RobustLock lock(queue.mutex);
    repairIfNeeded(queue);
    while (queue.count >= MAX_QUEUE) {
        const auto now = Clock::now();
        if (now >= deadline) {
            return false;
        }
        lock.wait(queue.notFull, deadline - now);
    }
    return pushLocked(queue, message);
}

std::optional<Message> popWait(MessageQueue& queue, std::chrono::nanoseconds timeout) {
    const auto deadline = Clock::now() + timeout;
    RobustLock lock(queue.mutex);
    repairIfNeeded(queue);
    while (queue.count == 0) {
        const auto now = Clock::now();
        if (now >= deadline) {
            return std::nullopt;
        }
        lock.wait(queue.notEmpty, deadline - now);
    }
    Message message = queue.messages[queue.head];
    queue.head = (queue.head + 1) % MAX_QUEUE;
    --queue.count;
    pthread_cond_signal(&queue.notFull);
    return message;
}

void clearQueue(MessageQueue& queue) {
    const RobustLock lock(queue.mutex);
    queue.head = 0;
    queue.count = 0;
    pthread_cond_broadcast(&queue.notFull);
}

int queueSize(MessageQueue& queue) {
    const RobustLock lock(queue.mutex);
    repairIfNeeded(queue);
    return queue.count;
}

}
