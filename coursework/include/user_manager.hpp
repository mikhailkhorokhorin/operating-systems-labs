#pragma once

#include <sys/types.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shared_memory.hpp"

namespace chat {

struct LoginResult {
    LoginStatus status;
    int slot;
};

bool isValidName(std::string_view name);

class UserManager {
public:
    explicit UserManager(SharedMemory& memory);

    LoginResult login(std::string_view name, pid_t pid);
    bool logout(int slot, std::string_view name);

    std::optional<int> findUser(std::string_view name) const;
    bool owns(int slot, std::string_view name) const;
    std::vector<int> activeSlots() const;
    std::vector<std::string> reapDisconnected();

private:
    SharedMemory* memory_;
};

}
