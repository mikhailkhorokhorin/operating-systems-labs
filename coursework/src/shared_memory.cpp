#include "shared_memory.hpp"

#include <unistd.h>

#include <cerrno>
#include <csignal>

#include "message_queue.hpp"
#include "sync.hpp"

namespace chat {

void initializeSharedMemory(SharedMemory& memory) {
    memory.magic.store(0, std::memory_order_relaxed);
    memory.serverPid = getpid();
    initRobustMutex(memory.usersMutex);
    for (auto& user : memory.users) {
        user = UserSlot{};
    }
    initRobustMutex(memory.logins.mutex);
    initSharedCondition(memory.logins.changed);
    for (auto& reply : memory.logins.replies) {
        reply = LoginReply{};
    }
    initializeQueue(memory.serverQueue);
    for (auto& queue : memory.clientQueues) {
        initializeQueue(queue);
    }
    memory.nextRequestId.store(0);
    memory.serverRunning.store(1);
    memory.version = VERSION;
    memory.magic.store(MAGIC, std::memory_order_release);
}

void destroySharedMemory(SharedMemory& memory) {
    memory.magic.store(0);
    memory.serverRunning.store(0);
    for (auto& queue : memory.clientQueues) {
        destroyQueue(queue);
    }
    destroyQueue(memory.serverQueue);
    pthread_cond_destroy(&memory.logins.changed);
    pthread_mutex_destroy(&memory.logins.mutex);
    pthread_mutex_destroy(&memory.usersMutex);
}

std::string_view describe(LoginStatus status) {
    switch (status) {
        case LoginStatus::Accepted:
            return "accepted";
        case LoginStatus::InvalidName:
            return "invalid name (1-31 printable characters without spaces and ':')";
        case LoginStatus::NameTaken:
            return "name is already taken";
        case LoginStatus::ServerFull:
            return "server is full";
        case LoginStatus::NoServer:
            return "server is not responding";
    }
    return "unknown status";
}

bool processAlive(pid_t pid) {
    return pid > 0 && (kill(pid, 0) == 0 || errno != ESRCH);
}

bool serverAlive(const SharedMemory& memory) {
    return memory.serverRunning.load() != 0 && processAlive(memory.serverPid);
}

}
