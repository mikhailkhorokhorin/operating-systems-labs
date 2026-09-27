#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "test_support/process.hpp"

namespace {

void runScripts(const std::string& program, const std::filesystem::path& dir) {
    const auto inputs = test_support::inputFiles(dir);
    ASSERT_FALSE(inputs.empty());
    for (const auto& input : inputs) {
        auto expected = input;
        expected.replace_extension(".out");
        const auto result = test_support::runProcess(program, {}, test_support::readFile(input));
        EXPECT_EQ(result.exitCode, 0) << input;
        EXPECT_EQ(result.out, test_support::readFile(expected)) << input;
        auto expectedErr = input;
        expectedErr.replace_extension(".err");
        EXPECT_EQ(result.err, test_support::readFile(expectedErr)) << input;
    }
}

}

TEST(EndToEndTest, StaticScripts) {
    runScripts(LAB4_STATIC_PATH, std::filesystem::path(TEST_DATA_DIR) / "static");
}

TEST(EndToEndTest, DynamicScripts) {
    runScripts(LAB4_DYNAMIC_PATH, std::filesystem::path(TEST_DATA_DIR) / "dynamic");
}

TEST(EndToEndTest, DynamicWorksFromAnotherDirectory) {
    const test_support::TempDir dir;
    const auto result = test_support::runProcess(LAB4_DYNAMIC_PATH, {}, "1 1 10\nq\n", dir.path());
    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(result.out, "Loaded libbasic.so\nPrimeCount = 4\n");
}

TEST(EndToEndTest, DynamicFailsWithoutLibraries) {
    const test_support::TempDir dir;
    const auto copy = dir.path() / "lab4_dynamic";
    std::filesystem::copy_file(LAB4_DYNAMIC_PATH, copy);
    const auto result = test_support::runProcess(copy.string(), {}, "q\n");
    EXPECT_EQ(result.exitCode, 1);
    EXPECT_NE(result.err.find("libbasic.so"), std::string::npos);
}
