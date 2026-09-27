#include <csignal>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "chat_server.hpp"
#include "shared_memory_manager.hpp"

namespace {

volatile std::sig_atomic_t stopSignal = 0;

}

extern "C" void onStopSignal(int) {
    stopSignal = 1;
}

namespace {

void installSignalHandlers() {
    struct sigaction action{};
    action.sa_handler = onStopSignal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

void printLine(const std::string& line) {
    std::cout << line << '\n' << std::flush;
}

}

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "usage: coursework_server [shm-name]\n";
        return 2;
    }
    try {
        const auto name = chat::resolveShmName(argc == 2 ? std::optional<std::string_view>(argv[1])
                                                         : std::nullopt);
        installSignalHandlers();
        const auto manager = chat::SharedMemoryManager::create(name);
        chat::ChatServer server(manager.memory(), printLine);
        server.run([] { return stopSignal != 0; });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
