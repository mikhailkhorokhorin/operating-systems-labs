#include "cli.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

struct CliResult {
    int code;
    std::string out;
    std::string err;
};

CliResult run(const std::vector<std::string>& args, const std::string& input,
              bool interactive = false) {
    std::istringstream in(input);
    std::ostringstream out;
    std::ostringstream err;
    const int code = runCli(args, in, out, err, interactive);
    return {code, out.str(), err.str()};
}

}

TEST(ParseThreadCountTest, AcceptsPositiveIntegers) {
    EXPECT_EQ(parseThreadCount("1"), 1U);
    EXPECT_EQ(parseThreadCount("16"), 16U);
    EXPECT_EQ(parseThreadCount(std::to_string(MAX_THREADS)), MAX_THREADS);
}

TEST(ParseThreadCountTest, RejectsInvalidValues) {
    EXPECT_EQ(parseThreadCount(""), std::nullopt);
    EXPECT_EQ(parseThreadCount("0"), std::nullopt);
    EXPECT_EQ(parseThreadCount("-3"), std::nullopt);
    EXPECT_EQ(parseThreadCount("4x"), std::nullopt);
    EXPECT_EQ(parseThreadCount("abc"), std::nullopt);
    EXPECT_EQ(parseThreadCount(std::to_string(MAX_THREADS + 1)), std::nullopt);
}

TEST(ReadLinearSystemTest, ReadsMatrixAndVector) {
    std::istringstream in("2\n1 2\n3 4\n5 6\n");
    const auto system = readLinearSystem(in, nullptr);
    ASSERT_TRUE(system.has_value());
    EXPECT_EQ(system->matrix, (Matrix{{1, 2}, {3, 4}}));
    EXPECT_EQ(system->rhs, (std::vector<double>{5, 6}));
}

TEST(ReadLinearSystemTest, RejectsBadInput) {
    const std::vector<std::string> inputs{
        "", "0", "-1", "x", "2\n1 2 3\n", "2\n1 2\n3 4\n5\n", std::to_string(MAX_SIZE + 1)};
    for (const auto& input : inputs) {
        std::istringstream in(input);
        EXPECT_EQ(readLinearSystem(in, nullptr), std::nullopt) << input;
    }
}

TEST(ReadLinearSystemTest, PrintsPrompts) {
    std::istringstream in("1\n2\n4\n");
    std::ostringstream prompt;
    ASSERT_TRUE(readLinearSystem(in, &prompt).has_value());
    EXPECT_NE(prompt.str().find("Enter system size"), std::string::npos);
    EXPECT_NE(prompt.str().find("right-hand side"), std::string::npos);
}

TEST(FormatSolutionTest, FormatsEachUnknown) {
    EXPECT_EQ(formatSolution({1.0, -0.0, 0.25}), "x[0] = 1\nx[1] = 0\nx[2] = 0.25\n");
}

TEST(RunCliTest, SolvesWithDefaultThreads) {
    const auto result = run({}, "2\n2 1\n1 3\n3 5\n");
    EXPECT_EQ(result.code, 0);
    EXPECT_EQ(result.out, "x[0] = 0.8\nx[1] = 1.4\n");
    EXPECT_EQ(result.err, "");
}

TEST(RunCliTest, InteractiveModePrintsThreadCount) {
    const auto result = run({"3"}, "1\n2\n4\n", true);
    EXPECT_EQ(result.code, 0);
    EXPECT_NE(result.out.find("Threads: 3"), std::string::npos);
    EXPECT_NE(result.out.find("x[0] = 2"), std::string::npos);
}

TEST(RunCliTest, RejectsBadThreadCount) {
    const auto result = run({"0"}, "1\n1\n1\n");
    EXPECT_EQ(result.code, 2);
    EXPECT_NE(result.err.find("thread count"), std::string::npos);
}

TEST(RunCliTest, RejectsExtraArguments) {
    const auto result = run({"1", "2"}, "");
    EXPECT_EQ(result.code, 2);
    EXPECT_NE(result.err.find("usage"), std::string::npos);
}

TEST(RunCliTest, ReportsInvalidInput) {
    const auto result = run({"2"}, "2\n1 2\n");
    EXPECT_EQ(result.code, 1);
    EXPECT_NE(result.err.find("expected size"), std::string::npos);
}

TEST(RunCliTest, ReportsSingularMatrix) {
    const auto result = run({"2"}, "2\n1 2\n2 4\n3 6\n");
    EXPECT_EQ(result.code, 1);
    EXPECT_EQ(result.err, "error: matrix is singular\n");
    EXPECT_EQ(result.out, "");
}
