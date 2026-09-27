#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "test_support/process.hpp"

TEST(EndToEndTest, MatchesExpectedOutput) {
    const auto inputs = test_support::inputFiles(TEST_DATA_DIR);
    ASSERT_FALSE(inputs.empty());
    for (const auto& input : inputs) {
        auto expected = input;
        expected.replace_extension(".out");
        for (const std::string threads : {"1", "2", "4"}) {
            const auto result =
                test_support::runProcess(LAB2_MAIN_PATH, {threads}, test_support::readFile(input));
            EXPECT_EQ(result.exitCode, 0) << input;
            EXPECT_EQ(result.out, test_support::readFile(expected)) << input;
            EXPECT_EQ(result.err, "") << input;
        }
    }
}

TEST(EndToEndTest, DefaultThreadCount) {
    const auto result = test_support::runProcess(LAB2_MAIN_PATH, {}, "1\n4\n2\n");
    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(result.out, "x[0] = 0.5\n");
}

TEST(EndToEndTest, RejectsNonPositiveThreadCount) {
    for (const std::string threads : {"0", "-2", "two"}) {
        const auto result = test_support::runProcess(LAB2_MAIN_PATH, {threads}, "1\n1\n1\n");
        EXPECT_EQ(result.exitCode, 2) << threads;
        EXPECT_NE(result.err.find("thread count"), std::string::npos) << threads;
    }
}

TEST(EndToEndTest, ReportsSingularMatrix) {
    const auto result = test_support::runProcess(LAB2_MAIN_PATH, {"2"}, "2\n1 2\n2 4\n3 6\n");
    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.err, "error: matrix is singular\n");
}
