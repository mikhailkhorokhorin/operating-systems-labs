#include "user_manager.hpp"

#include <algorithm>

#include "message_queue.hpp"
#include "sync.hpp"

namespace chat {

namespace {

bool validSlot(int slot) {
    return slot >= 0 && slot < MAX_USERS;
}

bool processGone(pid_t pid) {
    return pid > 0 && !processAlive(pid);
}

void releaseSlot(SharedMemory& memory, int slot) {
    memory.users[slot] = UserSlot{};
    clearQueue(memory.clientQueues[slot]);
}

}

bool isValidName(std::string_view name) {
    if (name.empty() || name.size() >= static_cast<std::size_t>(MAX_NAME)) {
        return false;
    }
    return std::ranges::all_of(
        name, [](char symbol) { return symbol > ' ' && symbol < '\x7f' && symbol != ':'; });
}

UserManager::UserManager(SharedMemory& memory) : memory_(&memory) {
}

LoginResult UserManager::login(std::string_view name, pid_t pid) {
    if (!isValidName(name)) {
        return {LoginStatus::InvalidName, -1};
    }
    const RobustLock lock(memory_->usersMutex);
    int freeSlot = -1;
    for (int slot = 0; slot < MAX_USERS; ++slot) {
        const auto& user = memory_->users[slot];
        if (user.active != 0) {
            if (view(user.name) == name) {
                return {LoginStatus::NameTaken, -1};
            }
        } else if (freeSlot < 0) {
            freeSlot = slot;
        }
    }
    if (freeSlot < 0) {
        return {LoginStatus::ServerFull, -1};
    }
    clearQueue(memory_->clientQueues[freeSlot]);
    auto& user = memory_->users[freeSlot];
    user.active = 1;
    user.pid = pid;
    copyBounded(user.name, name);
    return {LoginStatus::Accepted, freeSlot};
}

bool UserManager::logout(int slot, std::string_view name) {
    if (!validSlot(slot)) {
        return false;
    }
    const RobustLock lock(memory_->usersMutex);
    const auto& user = memory_->users[slot];
    if (user.active == 0 || view(user.name) != name) {
        return false;
    }
    releaseSlot(*memory_, slot);
    return true;
}

std::optional<int> UserManager::findUser(std::string_view name) const {
    if (name.empty()) {
        return std::nullopt;
    }
    const RobustLock lock(memory_->usersMutex);
    for (int slot = 0; slot < MAX_USERS; ++slot) {
        const auto& user = memory_->users[slot];
        if (user.active != 0 && view(user.name) == name) {
            return slot;
        }
    }
    return std::nullopt;
}

bool UserManager::owns(int slot, std::string_view name) const {
    if (!validSlot(slot)) {
        return false;
    }
    const RobustLock lock(memory_->usersMutex);
    const auto& user = memory_->users[slot];
    return user.active != 0 && view(user.name) == name;
}

std::vector<int> UserManager::activeSlots() const {
    std::vector<int> slots;
    const RobustLock lock(memory_->usersMutex);
    for (int slot = 0; slot < MAX_USERS; ++slot) {
        if (memory_->users[slot].active != 0) {
            slots.push_back(slot);
        }
    }
    return slots;
}

std::vector<std::string> UserManager::reapDisconnected() {
    std::vector<std::string> names;
    const RobustLock lock(memory_->usersMutex);
    for (int slot = 0; slot < MAX_USERS; ++slot) {
        const auto& user = memory_->users[slot];
        if (user.active != 0 && processGone(user.pid)) {
            names.emplace_back(view(user.name));
            releaseSlot(*memory_, slot);
        }
    }
    return names;
}

}
