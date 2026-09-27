#include "sync.hpp"

#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <system_error>

namespace chat {

void initRobustMutex(pthread_mutex_t& mutex) {
    pthread_mutexattr_t attributes{};
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_setpshared(&attributes, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attributes, PTHREAD_MUTEX_ROBUST);
    const int result = pthread_mutex_init(&mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);
    if (result != 0) {
        throw std::system_error(result, std::system_category(), "pthread_mutex_init");
    }
}

void initSharedCondition(pthread_cond_t& condition) {
    pthread_condattr_t attributes{};
    pthread_condattr_init(&attributes);
    pthread_condattr_setpshared(&attributes, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    const int result = pthread_cond_init(&condition, &attributes);
    pthread_condattr_destroy(&attributes);
    if (result != 0) {
        throw std::system_error(result, std::system_category(), "pthread_cond_init");
    }
}

RobustLock::RobustLock(pthread_mutex_t& mutex) noexcept : mutex_(&mutex) {
    check(pthread_mutex_lock(mutex_));
}

RobustLock::~RobustLock() {
    pthread_mutex_unlock(mutex_);
}

bool RobustLock::wait(pthread_cond_t& condition, std::chrono::nanoseconds timeout) noexcept {
    timespec deadline{};
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    deadline.tv_sec += static_cast<time_t>(seconds.count());
    deadline.tv_nsec += static_cast<long>((timeout - seconds).count());
    if (deadline.tv_nsec >= 1'000'000'000L) {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= 1'000'000'000L;
    }
    const int result = pthread_cond_timedwait(&condition, mutex_, &deadline);
    if (result == ETIMEDOUT) {
        return false;
    }
    check(result);
    return true;
}

void RobustLock::check(int result) noexcept {
    if (result == EOWNERDEAD) {
        pthread_mutex_consistent(mutex_);
        recovered_ = true;
        return;
    }
    if (result != 0) {
        std::abort();
    }
}

}
