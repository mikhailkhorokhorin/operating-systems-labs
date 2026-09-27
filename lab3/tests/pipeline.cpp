#include "pipeline.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <system_error>

#include "shared_memory.hpp"
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

TEST(ProcessSegmentTest, ReplacesInputWithSums) {
    auto memory = SharedMemory::create(uniqueSegmentName("lab3-test"), 0);
    writeMessage(memory, "1 2\n\n");
    processSegment(memory.name());
    EXPECT_EQ(readMessage(memory), "3\n0\n");
}

TEST(ProcessSegmentTest, MissingSegmentThrows) {
    EXPECT_THROW(processSegment(uniqueSegmentName("lab3-missing")), std::system_error);
}

TEST(RunPipelineTest, SumsFileThroughChild) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1 2\n3 4\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB3_CHILD_PATH, dir.path() / "input.txt", out, err), 0);
    EXPECT_EQ(out.str(), "3\n7\n");
    EXPECT_EQ(err.str(), "");
}

TEST(RunPipelineTest, HandlesInputLargerThanOnePage) {
    const test_support::TempDir dir;
    std::string input;
    std::string expected;
    for (int i = 0; i < 2000; ++i) {
        input += std::to_string(i) + " 1\n";
        expected += std::to_string(i + 1) + "\n";
    }
    test_support::writeFile(dir.path() / "input.txt", input);
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB3_CHILD_PATH, dir.path() / "input.txt", out, err), 0);
    EXPECT_EQ(out.str(), expected);
}

TEST(RunPipelineTest, ReportsMissingInput) {
    const test_support::TempDir dir;
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB3_CHILD_PATH, dir.path() / "missing.txt", out, err), 1);
    EXPECT_NE(err.str().find("cannot open"), std::string::npos);
}

TEST(RunPipelineTest, ReportsChildFailure) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1 oops\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(LAB3_CHILD_PATH, dir.path() / "input.txt", out, err), 1);
    EXPECT_NE(err.str().find("exit code 1"), std::string::npos);
    EXPECT_EQ(out.str(), "");
}

TEST(RunPipelineTest, ReportsMissingChild) {
    const test_support::TempDir dir;
    test_support::writeFile(dir.path() / "input.txt", "1\n");
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runPipeline(dir.path() / "no-such-child", dir.path() / "input.txt", out, err), 1);
    EXPECT_NE(err.str().find("exit code 127"), std::string::npos);
}
