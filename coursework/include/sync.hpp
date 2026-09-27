#pragma once

#include <pthread.h>

#include <chrono>

namespace chat {

void initRobustMutex(pthread_mutex_t& mutex);

void initSharedCondition(pthread_cond_t& condition);

class RobustLock {
public:
    explicit RobustLock(pthread_mutex_t& mutex) noexcept;
    ~RobustLock();

    RobustLock(const RobustLock&) = delete;
    RobustLock& operator=(const RobustLock&) = delete;

    bool wait(pthread_cond_t& condition, std::chrono::nanoseconds timeout) noexcept;

    bool recovered() const { return recovered_; }

private:
    void check(int result) noexcept;

    pthread_mutex_t* mutex_;
    bool recovered_ = false;
};

}
