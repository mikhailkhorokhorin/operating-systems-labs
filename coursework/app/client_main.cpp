#include <unistd.h>

#include <csignal>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "chat_client.hpp"
#include "line_reader.hpp"
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
        std::cerr << "usage: coursework_client [shm-name]\n";
        return 2;
    }
    try {
        const auto name = chat::resolveShmName(argc == 2 ? std::optional<std::string_view>(argv[1])
                                                         : std::nullopt);
        installSignalHandlers();
        const auto manager = chat::SharedMemoryManager::attach(name);
        if (isatty(STDIN_FILENO) != 0) {
            std::cout << "Enter a login: " << std::flush;
        }
        chat::LineReader reader(STDIN_FILENO);
        return chat::runClientSession(manager.memory(), reader, printLine,
                                      [] { return stopSignal != 0; });
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
