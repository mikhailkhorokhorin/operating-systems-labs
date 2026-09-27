#include "contract.hpp"

#include <gtest/gtest.h>

#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include "dynamic_library.hpp"

namespace {

struct Implementation {
    DynamicLibrary library;
    PrimeCountFunc primeCount;
    PiFunc pi;
};

Implementation load(const std::string& path) {
    DynamicLibrary library(path);
    const auto primeCount = library.function<PrimeCountFunc>("primeCount");
    const auto pi = library.function<PiFunc>("pi");
    return {std::move(library), primeCount, pi};
}

class ContractTest : public testing::TestWithParam<std::string> {};

}

TEST_P(ContractTest, CountsPrimesInRange) {
    const auto impl = load(GetParam());
    EXPECT_EQ(impl.primeCount(1, 10), 4);
    EXPECT_EQ(impl.primeCount(2, 2), 1);
    EXPECT_EQ(impl.primeCount(10, 20), 4);
    EXPECT_EQ(impl.primeCount(1, 1000), 168);
}

TEST_P(ContractTest, HandlesNegativeAndEmptyRanges) {
    const auto impl = load(GetParam());
    EXPECT_EQ(impl.primeCount(-100, 10), 4);
    EXPECT_EQ(impl.primeCount(-5, -1), 0);
    EXPECT_EQ(impl.primeCount(20, 10), 0);
    EXPECT_EQ(impl.primeCount(0, 1), 0);
}

TEST_P(ContractTest, ApproximatesPi) {
    const auto impl = load(GetParam());
    EXPECT_NEAR(impl.pi(100000), std::numbers::pi_v<float>, 1e-4F);
    EXPECT_FLOAT_EQ(impl.pi(0), 0.0F);
    EXPECT_FLOAT_EQ(impl.pi(-3), 0.0F);
}

INSTANTIATE_TEST_SUITE_P(Implementations, ContractTest,
                         testing::Values(std::string(LAB4_BASIC_PATH),
                                         std::string(LAB4_ADVANCED_PATH)));

TEST(ContractComparisonTest, ImplementationsAgreeOnPrimeCount) {
    const auto basic = load(LAB4_BASIC_PATH);
    const auto advanced = load(LAB4_ADVANCED_PATH);
    const std::vector<std::pair<int, int>> ranges{
        {1, 1}, {0, 100}, {50, 60}, {-10, 3}, {997, 1009}};
    for (const auto& [a, b] : ranges) {
        EXPECT_EQ(basic.primeCount(a, b), advanced.primeCount(a, b)) << a << " " << b;
    }
}

TEST(ContractComparisonTest, BothPiSeriesConverge) {
    const auto basic = load(LAB4_BASIC_PATH);
    const auto advanced = load(LAB4_ADVANCED_PATH);
    EXPECT_NEAR(basic.pi(200000), advanced.pi(200000), 1e-4F);
    EXPECT_FLOAT_EQ(basic.pi(1), 4.0F);
    EXPECT_NEAR(advanced.pi(1), 8.0F / 3.0F, 1e-6F);
}
