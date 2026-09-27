#include "shared_memory_manager.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "test_helpers.hpp"

namespace {

bool segmentExists(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDONLY, 0);
    if (fd < 0) {
        return false;
    }
    close(fd);
    return true;
}

void createRawSegment(const std::string& name, std::size_t size, pid_t serverPid) {
    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(ftruncate(fd, static_cast<off_t>(size)), 0);
    if (size >= sizeof(chat::SharedMemory)) {
        void* address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ASSERT_NE(address, MAP_FAILED);
        static_cast<chat::SharedMemory*>(address)->serverPid = serverPid;
        munmap(address, size);
    }
    close(fd);
}

}

TEST(SharedMemoryManagerTest, CreateAttachAndUnlink) {
    const auto name = chat_test::uniqueShmName("manager");
    {
        const auto server = chat::SharedMemoryManager::create(name);
        EXPECT_TRUE(server.owner());
        EXPECT_EQ(server.name(), name);
        const auto client = chat::SharedMemoryManager::attach(name);
        EXPECT_FALSE(client.owner());
        client.memory().nextRequestId.store(42);
        EXPECT_EQ(server.memory().nextRequestId.load(), 42);
    }
    EXPECT_FALSE(segmentExists(name));
}

TEST(SharedMemoryManagerTest, SecondServerIsRejected) {
    const auto name = chat_test::uniqueShmName("manager");
    const auto server = chat::SharedMemoryManager::create(name);
    EXPECT_THROW(chat::SharedMemoryManager::create(name), std::runtime_error);
    EXPECT_EQ(server.memory().magic.load(), chat::MAGIC);
}

TEST(SharedMemoryManagerTest, StaleSegmentOfDeadServerIsReplaced) {
    const auto name = chat_test::uniqueShmName("manager");
    createRawSegment(name, sizeof(chat::SharedMemory), chat_test::deadPid());
    const auto server = chat::SharedMemoryManager::create(name);
    EXPECT_EQ(server.memory().serverPid, getpid());
}

TEST(SharedMemoryManagerTest, ForeignSegmentIsNotRemoved) {
    const auto name = chat_test::uniqueShmName("manager");
    createRawSegment(name, 16, 0);
    EXPECT_THROW(chat::SharedMemoryManager::create(name), std::runtime_error);
    EXPECT_THROW(chat::SharedMemoryManager::attach(name), std::runtime_error);
    shm_unlink(name.c_str());
}

TEST(SharedMemoryManagerTest, AttachRejectsUninitializedSegment) {
    const auto name = chat_test::uniqueShmName("manager");
    createRawSegment(name, sizeof(chat::SharedMemory), getpid());
    EXPECT_THROW(chat::SharedMemoryManager::attach(name), std::runtime_error);
    shm_unlink(name.c_str());
}

TEST(SharedMemoryManagerTest, AttachWithoutServerFails) {
    EXPECT_THROW(chat::SharedMemoryManager::attach(chat_test::uniqueShmName("missing")),
                 std::runtime_error);
    EXPECT_THROW(chat::SharedMemoryManager::attach("/bad/name"), std::system_error);
}

TEST(SharedMemoryManagerTest, CreateReportsInvalidName) {
    EXPECT_THROW(chat::SharedMemoryManager::create("/bad/name"), std::system_error);
}

TEST(SharedMemoryManagerTest, MoveTransfersOwnership) {
    const auto first = chat_test::uniqueShmName("manager");
    const auto second = chat_test::uniqueShmName("manager");
    auto a = chat::SharedMemoryManager::create(first);
    auto b = chat::SharedMemoryManager::create(second);
    b = std::move(a);
    EXPECT_FALSE(segmentExists(second));
    EXPECT_EQ(b.name(), first);
    const chat::SharedMemoryManager c(std::move(b));
    EXPECT_TRUE(c.owner());
    EXPECT_TRUE(segmentExists(first));
}

TEST(ResolveShmNameTest, PrefersArgumentThenEnvironment) {
    unsetenv(chat::SHM_NAME_ENV);
    EXPECT_EQ(chat::resolveShmName(std::nullopt), chat::DEFAULT_SHM_NAME);
    EXPECT_EQ(chat::resolveShmName("room"), "/room");
    EXPECT_EQ(chat::resolveShmName("/room"), "/room");
    setenv(chat::SHM_NAME_ENV, "from-env", 1);
    EXPECT_EQ(chat::resolveShmName(std::nullopt), "/from-env");
    EXPECT_EQ(chat::resolveShmName(""), "/from-env");
    EXPECT_EQ(chat::resolveShmName("arg"), "/arg");
    unsetenv(chat::SHM_NAME_ENV);
}
