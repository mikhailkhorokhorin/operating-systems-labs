#include "shared_memory.hpp"

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "sync.hpp"
#include "test_helpers.hpp"

TEST(CopyBoundedTest, CopiesShortStrings) {
    char buffer[8];
    EXPECT_TRUE(chat::copyBounded(buffer, "abc"));
    EXPECT_EQ(chat::view(buffer), "abc");
}

TEST(CopyBoundedTest, TruncatesAndTerminates) {
    char buffer[4];
    EXPECT_FALSE(chat::copyBounded(buffer, "abcdef"));
    EXPECT_EQ(chat::view(buffer), "abc");
    EXPECT_EQ(buffer[3], '\0');
}

TEST(CopyBoundedTest, HandlesEmptySource) {
    char buffer[4] = {'x', 'y', 'z', 'w'};
    EXPECT_TRUE(chat::copyBounded(buffer, ""));
    EXPECT_EQ(chat::view(buffer), "");
}

TEST(ViewTest, StopsAtFieldEndWithoutTerminator) {
    const char buffer[3] = {'a', 'b', 'c'};
    EXPECT_EQ(chat::view(buffer), "abc");
}

TEST(SharedMemoryTest, InitializationSetsHeader) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    EXPECT_EQ(memory.magic.load(), chat::MAGIC);
    EXPECT_EQ(memory.version, chat::VERSION);
    EXPECT_EQ(memory.serverPid, getpid());
    EXPECT_EQ(memory.serverRunning.load(), 1);
    EXPECT_EQ(memory.users[0].active, 0);
}

TEST(SharedMemoryTest, ServerAliveNeedsFlagAndLiveProcess) {
    const chat_test::LocalMemory local;
    auto& memory = local.get();
    EXPECT_TRUE(chat::processAlive(getpid()));
    EXPECT_FALSE(chat::processAlive(0));
    EXPECT_TRUE(chat::serverAlive(memory));
    memory.serverPid = chat_test::deadPid();
    EXPECT_FALSE(chat::processAlive(memory.serverPid));
    EXPECT_FALSE(chat::serverAlive(memory));
    memory.serverPid = getpid();
    memory.serverRunning.store(0);
    EXPECT_FALSE(chat::serverAlive(memory));
}

TEST(DescribeTest, EveryStatusHasText) {
    for (const auto status :
         {chat::LoginStatus::Accepted, chat::LoginStatus::InvalidName, chat::LoginStatus::NameTaken,
          chat::LoginStatus::ServerFull, chat::LoginStatus::NoServer}) {
        EXPECT_FALSE(chat::describe(status).empty());
    }
    EXPECT_EQ(chat::describe(chat::LoginStatus::NameTaken), "name is already taken");
}

TEST(RobustLockTest, RecoversMutexOfDeadOwner) {
    void* address = mmap(nullptr, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(address, MAP_FAILED);
    auto* mutex = static_cast<pthread_mutex_t*>(address);
    chat::initRobustMutex(*mutex);

    const pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
        pthread_mutex_lock(mutex);
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);

    {
        const chat::RobustLock lock(*mutex);
        EXPECT_TRUE(lock.recovered());
    }
    {
        const chat::RobustLock lock(*mutex);
        EXPECT_FALSE(lock.recovered());
    }
    pthread_mutex_destroy(mutex);
    munmap(address, sizeof(pthread_mutex_t));
}

TEST(RobustLockTest, WaitTimesOut) {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    chat::initRobustMutex(mutex);
    chat::initSharedCondition(condition);
    {
        chat::RobustLock lock(mutex);
        EXPECT_FALSE(lock.wait(condition, std::chrono::milliseconds(20)));
        EXPECT_FALSE(lock.wait(condition, std::chrono::milliseconds(1500)));
    }
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
}
