#include "cli.hpp"

#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <system_error>

std::optional<std::size_t> parseThreadCount(std::string_view text) {
    std::size_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    if (value == 0 || value > MAX_THREADS) {
        return std::nullopt;
    }
    return value;
}

std::optional<LinearSystem> readLinearSystem(std::istream& in, std::ostream* prompt) {
    if (prompt != nullptr) {
        *prompt << "Enter system size: " << std::flush;
    }
    long long size = 0;
    if (!(in >> size) || size <= 0 || static_cast<std::size_t>(size) > MAX_SIZE) {
        return std::nullopt;
    }
    const auto count = static_cast<std::size_t>(size);
    LinearSystem result{Matrix(count, std::vector<double>(count)), std::vector<double>(count)};
    if (prompt != nullptr) {
        *prompt << "Enter the matrix row by row:\n" << std::flush;
    }
    for (auto& row : result.matrix) {
        for (double& value : row) {
            if (!(in >> value)) {
                return std::nullopt;
            }
        }
    }
    if (prompt != nullptr) {
        *prompt << "Enter the right-hand side:\n" << std::flush;
    }
    for (double& value : result.rhs) {
        if (!(in >> value)) {
            return std::nullopt;
        }
    }
    return result;
}

std::string formatSolution(const std::vector<double>& solution) {
    std::string text;
    for (std::size_t index = 0; index < solution.size(); ++index) {
        text += std::format("x[{}] = {:.10g}\n", index, solution[index] + 0.0);
    }
    return text;
}

int runCli(const std::vector<std::string>& args, std::istream& in, std::ostream& out,
           std::ostream& err, bool interactive) {
    std::size_t threads = DEFAULT_THREADS;
    if (args.size() > 1) {
        err << "usage: lab2_main [threads]\n";
        return 2;
    }
    if (args.size() == 1) {
        const auto parsed = parseThreadCount(args.front());
        if (!parsed) {
            err << std::format("error: thread count must be an integer in [1, {}]\n", MAX_THREADS);
            return 2;
        }
        threads = *parsed;
    }
    if (interactive) {
        out << "Threads: " << threads << '\n';
    }

    const auto input = readLinearSystem(in, interactive ? &out : nullptr);
    if (!input) {
        err << std::format("error: expected size in [1, {}], then the matrix and the vector\n",
                           MAX_SIZE);
        return 1;
    }
    try {
        out << formatSolution(solveLinearSystem(input->matrix, input->rhs, threads));
    } catch (const SingularMatrixError& error) {
        err << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
