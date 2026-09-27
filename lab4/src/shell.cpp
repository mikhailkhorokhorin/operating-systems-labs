#include "shell.hpp"

#include <exception>
#include <istream>
#include <ostream>
#include <sstream>

namespace {

bool atEnd(std::istringstream& stream) {
    std::string extra;
    return !(stream >> extra);
}

void execute(std::istringstream& stream, const std::string& command, std::ostream& out,
             std::ostream& err, Backend& backend) {
    if (command == "0") {
        if (!atEnd(stream)) {
            err << "error: usage: 0\n";
            return;
        }
        out << backend.toggle() << '\n';
    } else if (command == "1") {
        int a = 0;
        int b = 0;
        if (!(stream >> a >> b) || !atEnd(stream)) {
            err << "error: usage: 1 A B\n";
            return;
        }
        out << "PrimeCount = " << backend.primeCount(a, b) << '\n';
    } else if (command == "2") {
        int k = 0;
        if (!(stream >> k) || !atEnd(stream)) {
            err << "error: usage: 2 K\n";
            return;
        }
        out << "Pi = " << backend.pi(k) << '\n';
    } else {
        err << "error: unknown command '" << command << "'\n";
    }
}

}

int runShell(std::istream& in, std::ostream& out, std::ostream& err, Backend& backend,
             bool interactive) {
    std::string line;
    while (true) {
        if (interactive) {
            out << "> " << std::flush;
        }
        if (!std::getline(in, line)) {
            break;
        }
        std::istringstream stream(line);
        std::string command;
        if (!(stream >> command)) {
            continue;
        }
        if (command == "q") {
            break;
        }
        try {
            execute(stream, command, out, err, backend);
        } catch (const std::exception& error) {
            err << "error: " << error.what() << '\n';
        }
        out.flush();
    }
    if (interactive) {
        out << '\n';
    }
    return 0;
}
