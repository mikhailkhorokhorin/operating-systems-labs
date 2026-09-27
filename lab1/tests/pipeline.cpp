#include "pipeline.hpp"

#include <gtest/gtest.h>

#include <sstream>

#include "test_support/process.hpp"

TEST(ReadFileNameTest, TrimsWhitespace) {
    std::istringstream in("  data.txt \r\n");
    EXPECT_EQ(readFileName(in), "data.txt");
}

TEST(ReadFileNameTest, RejectsEmptyInput) {
    std::istringstream empty;
    EXPECT_EQ(readFileName(empty), std::nullopt);
    std::istringstream blank("   \n");
    EXPECT_EQ(readFileName(blank), std::nullopt);
}

TEST(RunPipelineTest, SumsFileThroughChild) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1 2\n3 4\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB1_CHILD_PATH, dir.path() / "input.txt", out, err), 0);
    EXPECT_EQ(out.str(), "3\n7\n");
    EXPECT_EQ(err.str(), "");
}

TEST(RunPipelineTest, ReportsMissingInput) {
    const test_support::TempDir dir;
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB1_CHILD_PATH, dir.path() / "missing.txt", out, err), 1);
    EXPECT_NE(err.str().find("cannot open"), std::string::npos);
}

TEST(RunPipelineTest, ReportsChildFailure) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1 oops\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB1_CHILD_PATH, dir.path() / "input.txt", out, err), 1);
    EXPECT_NE(err.str().find("exit code 1"), std::string::npos);
}

TEST(RunPipelineTest, ReportsMissingChild) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(dir.path() / "no-such-child", dir.path() / "input.txt", out, err), 1);
    EXPECT_NE(err.str().find("exit code 127"), std::string::npos);
}
