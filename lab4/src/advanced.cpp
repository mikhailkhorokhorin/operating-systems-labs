#include <algorithm>
#include <cstddef>
#include <vector>

#include "contract.hpp"

extern "C" int primeCount(int a, int b) {
    const int low = std::max(a, 2);
    if (b < low) {
        return 0;
    }
    const auto high = static_cast<std::size_t>(b);
    std::vector<bool> composite(high + 1, false);
    for (std::size_t number = 2; number <= high / number; ++number) {
        if (!composite[number]) {
            for (std::size_t multiple = number * number; multiple <= high; multiple += number) {
                composite[multiple] = true;
            }
        }
    }
    int count = 0;
    for (auto number = static_cast<std::size_t>(low); number <= high; ++number) {
        if (!composite[number]) {
            ++count;
        }
    }
    return count;
}

extern "C" float pi(int k) {
    if (k < 1) {
        return 0.0F;
    }
    double product = 1.0;
    for (int term = 1; term <= k; ++term) {
        const double square = 4.0 * term * term;
        product *= square / (square - 1.0);
    }
    return static_cast<float>(2.0 * product);
}
