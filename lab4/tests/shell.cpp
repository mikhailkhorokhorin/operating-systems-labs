#include "shell.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace {

class FakeBackend final : public Backend {
public:
    int primeCount(int a, int b) override { return a + b; }
    float pi(int k) override { return static_cast<float>(k) / 2.0F; }
    std::string toggle() override {
        ++toggles;
        if (toggles > 1) {
            throw std::runtime_error("toggle failed");
        }
        return "toggled";
    }

    int toggles = 0;
};

struct ShellResult {
    std::string out;
    std::string err;
    int toggles;
};

ShellResult run(const std::string& input, bool interactive = false) {
    FakeBackend backend;
    std::istringstream in(input);
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runShell(in, out, err, backend, interactive), 0);
    return {out.str(), err.str(), backend.toggles};
}

}

TEST(ShellTest, RunsCommands) {
    const auto result = run("1 2 3\n2 5\n0\n");
    EXPECT_EQ(result.out, "PrimeCount = 5\nPi = 2.5\ntoggled\n");
    EXPECT_EQ(result.err, "");
    EXPECT_EQ(result.toggles, 1);
}

TEST(ShellTest, StopsOnQuit) {
    const auto result = run("1 1 1\nq\n1 2 2\n");
    EXPECT_EQ(result.out, "PrimeCount = 2\n");
}

TEST(ShellTest, StopsOnEndOfInput) {
    const auto result = run("2 4");
    EXPECT_EQ(result.out, "Pi = 2\n");
}

TEST(ShellTest, SkipsBlankLines) {
    const auto result = run("\n   \n2 2\n");
    EXPECT_EQ(result.out, "Pi = 1\n");
    EXPECT_EQ(result.err, "");
}

TEST(ShellTest, ReportsBadCommands) {
    const auto result = run("7\n1 2\n1 a b\n2\n2 1 1\n0 1\n");
    EXPECT_EQ(result.out, "");
    EXPECT_EQ(result.err,
              "error: unknown command '7'\n"
              "error: usage: 1 A B\n"
              "error: usage: 1 A B\n"
              "error: usage: 2 K\n"
              "error: usage: 2 K\n"
              "error: usage: 0\n");
}

TEST(ShellTest, ReportsBackendErrors) {
    const auto result = run("0\n0\n2 2\n");
    EXPECT_EQ(result.out, "toggled\nPi = 1\n");
    EXPECT_EQ(result.err, "error: toggle failed\n");
}

TEST(ShellTest, InteractiveModePrintsPrompts) {
    const auto result = run("2 2\n", true);
    EXPECT_EQ(result.out, "> Pi = 1\n> \n");
}
