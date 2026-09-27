#include "line_sum.hpp"

#include <charconv>
#include <cmath>
#include <format>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {

bool isSpace(char symbol) {
    return symbol == ' ' || symbol == '\t' || symbol == '\r' || symbol == '\n' || symbol == '\v' ||
           symbol == '\f';
}

double parseNumber(std::string_view token) {
    std::string_view digits = token;
    if (digits.size() > 1 && digits.front() == '+') {
        digits.remove_prefix(1);
    }
    double value = 0.0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (error != std::errc{} || end != digits.data() + digits.size() || !std::isfinite(value)) {
        throw std::invalid_argument(std::format("invalid number '{}'", token));
    }
    return value;
}

}

double sumLine(std::string_view line) {
    double sum = 0.0;
    std::size_t position = 0;
    while (position < line.size()) {
        while (position < line.size() && isSpace(line[position])) {
            ++position;
        }
        const std::size_t start = position;
        while (position < line.size() && !isSpace(line[position])) {
            ++position;
        }
        if (position > start) {
            sum += parseNumber(line.substr(start, position - start));
        }
    }
    if (!std::isfinite(sum)) {
        throw std::invalid_argument("sum is out of range");
    }
    return sum;
}

std::string formatNumber(double value) {
    return std::format("{:.15g}", value + 0.0);
}

std::string sumText(std::string_view text) {
    std::string result;
    std::size_t lineNumber = 0;
    while (!text.empty()) {
        ++lineNumber;
        const auto end = text.find('\n');
        const auto line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        try {
            result += formatNumber(sumLine(line));
            result += '\n';
        } catch (const std::invalid_argument& error) {
            throw std::invalid_argument(std::format("line {}: {}", lineNumber, error.what()));
        }
    }
    return result;
}
