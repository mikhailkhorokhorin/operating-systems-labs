#include <algorithm>

#include "contract.hpp"

static bool isPrime(int number) {
    if (number < 2) {
        return false;
    }
    for (int divisor = 2; divisor <= number / divisor; ++divisor) {
        if (number % divisor == 0) {
            return false;
        }
    }
    return true;
}

extern "C" int primeCount(int a, int b) {
    int count = 0;
    for (long long number = std::max(a, 2); number <= b; ++number) {
        if (isPrime(static_cast<int>(number))) {
            ++count;
        }
    }
    return count;
}

extern "C" float pi(int k) {
    double sum = 0.0;
    for (int term = 0; term < k; ++term) {
        const double sign = term % 2 == 0 ? 1.0 : -1.0;
        sum += sign / (2.0 * term + 1.0);
    }
    return static_cast<float>(4.0 * sum);
}
