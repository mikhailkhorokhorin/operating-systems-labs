#include "shared_memory.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

bool segmentExists(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDONLY, 0);
    if (fd < 0) {
        return false;
    }
    close(fd);
    return true;
}

}

TEST(SharedMemoryTest, OwnerUnlinksSegment) {
    const auto name = uniqueSegmentName("lab3-test");
    {
        const auto memory = SharedMemory::create(name, 16);
        EXPECT_EQ(memory.size(), 16U);
        EXPECT_TRUE(memory.owner());
        EXPECT_EQ(memory.name(), name);
        EXPECT_TRUE(segmentExists(name));
    }
    EXPECT_FALSE(segmentExists(name));
}

TEST(SharedMemoryTest, CreateFailsWhenSegmentExists) {
    const auto name = uniqueSegmentName("lab3-test");
    const auto memory = SharedMemory::create(name, 8);
    EXPECT_THROW(SharedMemory::create(name, 8), std::system_error);
}

TEST(SharedMemoryTest, OpenFailsForMissingSegment) {
    EXPECT_THROW(SharedMemory::open(uniqueSegmentName("lab3-missing")), std::system_error);
}

TEST(SharedMemoryTest, SecondMappingSeesWrites) {
    const auto name = uniqueSegmentName("lab3-test");
    auto owner = SharedMemory::create(name, 0);
    writeMessage(owner, "hello");
    auto reader = SharedMemory::open(name);
    EXPECT_FALSE(reader.owner());
    EXPECT_EQ(readMessage(reader), "hello");
    writeMessage(reader, "a much longer reply than before");
    EXPECT_EQ(readMessage(owner), "a much longer reply than before");
}

TEST(SharedMemoryTest, EmptyMessageRoundTrips) {
    auto memory = SharedMemory::create(uniqueSegmentName("lab3-test"), 0);
    writeMessage(memory, "");
    EXPECT_EQ(readMessage(memory), "");
}

TEST(SharedMemoryTest, ReadRejectsMissingHeader) {
    auto memory = SharedMemory::create(uniqueSegmentName("lab3-test"), 4);
    EXPECT_THROW(readMessage(memory), std::runtime_error);
}

TEST(SharedMemoryTest, ReadRejectsTruncatedPayload) {
    auto memory = SharedMemory::create(uniqueSegmentName("lab3-test"), 12);
    const std::uint64_t length = 100;
    std::memcpy(memory.bytes().data(), &length, sizeof(length));
    EXPECT_THROW(readMessage(memory), std::runtime_error);
}

TEST(SharedMemoryTest, MoveTransfersOwnership) {
    const auto name = uniqueSegmentName("lab3-test");
    auto first = SharedMemory::create(name, 8);
    SharedMemory second(std::move(first));
    EXPECT_TRUE(second.owner());
    auto third = SharedMemory::create(uniqueSegmentName("lab3-test"), 8);
    const auto thirdName = third.name();
    third = std::move(second);
    EXPECT_FALSE(segmentExists(thirdName));
    EXPECT_EQ(third.name(), name);
    EXPECT_TRUE(segmentExists(name));
}

TEST(SharedMemoryTest, UniqueNamesDiffer) {
    EXPECT_NE(uniqueSegmentName("x"), uniqueSegmentName("x"));
    EXPECT_EQ(uniqueSegmentName("x").front(), '/');
}
