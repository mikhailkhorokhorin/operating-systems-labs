#pragma once

#include <pthread.h>
#include <sys/types.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace chat {

inline constexpr int MAX_USERS = 8;
inline constexpr int MAX_QUEUE = 32;
inline constexpr int MAX_TEXT = 256;
inline constexpr int MAX_NAME = 32;
inline constexpr int MAX_PENDING_LOGINS = 16;
inline constexpr std::size_t MAX_HISTORY = 256;
inline constexpr std::uint32_t MAGIC = 0x43484154;
inline constexpr std::uint32_t VERSION = 3;
inline constexpr std::string_view DEFAULT_SHM_NAME = "/chat_shm";
inline constexpr const char* SHM_NAME_ENV = "CHAT_SHM_NAME";
inline constexpr std::chrono::milliseconds POLL_INTERVAL{50};
inline constexpr std::chrono::milliseconds REAP_INTERVAL{1000};
inline constexpr std::chrono::milliseconds LOGIN_TIMEOUT{5000};

enum class MessageKind : std::uint8_t { Login, Logout, Text, History, System, Shutdown };

enum class LoginStatus : std::uint8_t { Accepted, InvalidName, NameTaken, ServerFull, NoServer };

struct Message {
    MessageKind kind;
    std::int32_t slot;
    std::int32_t requestId;
    pid_t pid;
    char from[MAX_NAME];
    char to[MAX_NAME];
    char text[MAX_TEXT];
};

struct MessageQueue {
    pthread_mutex_t mutex;
    pthread_cond_t notEmpty;
    pthread_cond_t notFull;
    std::int32_t head;
    std::int32_t count;
    Message messages[MAX_QUEUE];
};

struct LoginReply {
    std::int32_t requestId;
    std::int32_t slot;
    LoginStatus status;
    pid_t pid;
};

struct LoginBoard {
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    LoginReply replies[MAX_PENDING_LOGINS];
};

struct UserSlot {
    pid_t pid;
    std::int32_t active;
    char name[MAX_NAME];
};

struct SharedMemory {
    std::atomic<std::uint32_t> magic;
    std::uint32_t version;
    pid_t serverPid;
    std::atomic<std::int32_t> serverRunning;
    std::atomic<std::int32_t> nextRequestId;
    pthread_mutex_t usersMutex;
    UserSlot users[MAX_USERS];
    LoginBoard logins;
    MessageQueue serverQueue;
    MessageQueue clientQueues[MAX_USERS];
};

static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
static_assert(std::atomic<std::int32_t>::is_always_lock_free);

void initializeSharedMemory(SharedMemory& memory);

void destroySharedMemory(SharedMemory& memory);

std::string_view describe(LoginStatus status);

bool processAlive(pid_t pid);

bool serverAlive(const SharedMemory& memory);

template <std::size_t N>
bool copyBounded(char (&target)[N], std::string_view source) {
    const std::size_t length = std::min(source.size(), N - 1);
    if (length > 0) {
        std::memcpy(static_cast<char*>(target), source.data(), length);
    }
    target[length] = '\0';
    return length == source.size();
}

template <std::size_t N>
std::string_view view(const char (&field)[N]) {
    const char* begin = static_cast<const char*>(field);
    const char* end = std::ranges::find(begin, begin + N, '\0');
    return {begin, static_cast<std::size_t>(end - begin)};
}

}
