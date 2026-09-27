#include <gtest/gtest.h>

#include <filesystem>

#include "test_support/process.hpp"

TEST(EndToEndTest, MatchesExpectedOutput) {
    const auto inputs = test_support::inputFiles(TEST_DATA_DIR);
    ASSERT_FALSE(inputs.empty());
    for (const auto& input : inputs) {
        auto expected = input;
        expected.replace_extension(".out");
        const auto result = test_support::runProcess(LAB1_MAIN_PATH, {}, input.string() + "\n");
        EXPECT_EQ(result.exitCode, 0) << input;
        EXPECT_EQ(result.out, test_support::readFile(expected)) << input;
        EXPECT_EQ(result.err, "") << input;
    }
}

TEST(EndToEndTest, FailsOnMissingFile) {
    const auto result = test_support::runProcess(LAB1_MAIN_PATH, {}, "/nonexistent/input.txt\n");
    EXPECT_EQ(result.exitCode, 1);
    EXPECT_NE(result.err.find("cannot open"), std::string::npos);
}

TEST(EndToEndTest, FailsWithoutFileName) {
    const auto result = test_support::runProcess(LAB1_MAIN_PATH, {}, "");
    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.err, "error: no file name given\n");
}

TEST(EndToEndTest, ReportsInvalidNumber) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "bad.txt", "1 2\nx\n");
    const auto result =
        test_support::runProcess(LAB1_MAIN_PATH, {}, (dir.path() / "bad.txt").string() + "\n");
    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.out, "3\n");
    EXPECT_NE(result.err.find("line 2: invalid number 'x'"), std::string::npos);
}
