#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "gauss_jordan.hpp"

inline constexpr std::size_t DEFAULT_THREADS = 2;
inline constexpr std::size_t MAX_THREADS = 256;
inline constexpr std::size_t MAX_SIZE = 2000;

struct LinearSystem {
    Matrix matrix;
    std::vector<double> rhs;
};

std::optional<std::size_t> parseThreadCount(std::string_view text);

std::optional<LinearSystem> readLinearSystem(std::istream& in, std::ostream* prompt);

std::string formatSolution(const std::vector<double>& solution);

int runCli(const std::vector<std::string>& args, std::istream& in, std::ostream& out,
           std::ostream& err, bool interactive);
