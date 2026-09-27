#include "process.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>

#include <filesystem>
#include <utility>

TEST(ProcessTest, SiblingExecutableIsNextToCurrentExecutable) {
    const auto sibling = siblingExecutable("lab1_child");
    EXPECT_EQ(sibling.parent_path(), executableDirectory());
    EXPECT_TRUE(std::filesystem::exists(sibling));
}

TEST(ProcessTest, ExitCodeOfNormalExit) {
    const pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
        _exit(3);
    }
    EXPECT_EQ(waitForChild(pid), 3);
}

TEST(ProcessTest, ExitCodeOfSignal) {
    const pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
        raise(SIGKILL);
        _exit(0);
    }
    EXPECT_EQ(waitForChild(pid), 128 + SIGKILL);
}

TEST(ProcessTest, WaitForUnknownChildThrows) {
    EXPECT_THROW(waitForChild(-12345), std::system_error);
}

TEST(FileDescriptorTest, ClosesAndMoves) {
    FileDescriptor first(open("/dev/null", O_RDONLY | O_CLOEXEC));
    ASSERT_TRUE(first.valid());
    const int raw = first.get();
    FileDescriptor second(std::move(first));
    EXPECT_EQ(second.get(), raw);
    FileDescriptor third;
    EXPECT_FALSE(third.valid());
    third = std::move(second);
    EXPECT_EQ(third.get(), raw);
    third.reset();
    EXPECT_FALSE(third.valid());
    EXPECT_EQ(fcntl(raw, F_GETFD), -1);
}
