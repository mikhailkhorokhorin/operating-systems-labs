#include "line_reader.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <string>
#include <string_view>

namespace {

constexpr std::chrono::milliseconds SHORT{10};

class Pipe {
public:
    Pipe() {
        if (pipe2(fds_.data(), O_CLOEXEC) != 0) {
            throw std::runtime_error("pipe failed");
        }
    }

    ~Pipe() {
        closeWrite();
        close(fds_[0]);
    }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    int readEnd() const { return fds_[0]; }

    void write(std::string_view text) const {
        ASSERT_EQ(::write(fds_[1], text.data(), text.size()), static_cast<ssize_t>(text.size()));
    }

    void closeWrite() {
        if (fds_[1] >= 0) {
            close(fds_[1]);
            fds_[1] = -1;
        }
    }

private:
    std::array<int, 2> fds_{-1, -1};
};

}

TEST(LineReaderTest, SplitsLinesAndStripsCarriageReturn) {
    Pipe pipe;
    chat::LineReader reader(pipe.readEnd());
    pipe.write("first\r\nsecond\nthi");
    std::string line;
    ASSERT_EQ(reader.read(line, SHORT), chat::ReadStatus::Line);
    EXPECT_EQ(line, "first");
    ASSERT_EQ(reader.read(line, SHORT), chat::ReadStatus::Line);
    EXPECT_EQ(line, "second");
    EXPECT_EQ(reader.read(line, SHORT), chat::ReadStatus::Timeout);
    pipe.write("rd\n");
    ASSERT_EQ(reader.read(line, SHORT), chat::ReadStatus::Line);
    EXPECT_EQ(line, "third");
}

TEST(LineReaderTest, ReturnsPartialLineThenEof) {
    Pipe pipe;
    chat::LineReader reader(pipe.readEnd());
    pipe.write("tail");
    pipe.closeWrite();
    std::string line;
    ASSERT_EQ(reader.read(line, SHORT), chat::ReadStatus::Line);
    EXPECT_EQ(line, "tail");
    EXPECT_EQ(reader.read(line, SHORT), chat::ReadStatus::Eof);
    EXPECT_EQ(reader.read(line, SHORT), chat::ReadStatus::Eof);
}

TEST(LineReaderTest, TimesOutWithoutInput) {
    Pipe pipe;
    chat::LineReader reader(pipe.readEnd());
    std::string line;
    EXPECT_EQ(reader.read(line, SHORT), chat::ReadStatus::Timeout);
}

TEST(LineReaderTest, EmptyInputIsEof) {
    Pipe pipe;
    pipe.closeWrite();
    chat::LineReader reader(pipe.readEnd());
    std::string line;
    EXPECT_EQ(reader.read(line, SHORT), chat::ReadStatus::Eof);
}
