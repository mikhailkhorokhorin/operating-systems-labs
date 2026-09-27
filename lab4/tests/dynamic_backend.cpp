#include "dynamic_backend.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "dynamic_library.hpp"

TEST(DynamicLibraryTest, ResolvesBareNamesNextToExecutable) {
    const auto resolved = resolveLibraryPath("libbasic.so");
    EXPECT_EQ(std::filesystem::path(resolved), std::filesystem::path(LAB4_BASIC_PATH));
    EXPECT_EQ(resolveLibraryPath("libnone.so"), "libnone.so");
    EXPECT_EQ(resolveLibraryPath("/opt/lib/libx.so"), "/opt/lib/libx.so");
    const DynamicLibrary library("libbasic.so");
    EXPECT_EQ(library.path(), "libbasic.so");
}

TEST(DynamicLibraryTest, ThrowsForMissingLibrary) {
    EXPECT_THROW(DynamicLibrary("libdoes-not-exist.so"), std::runtime_error);
}

TEST(DynamicLibraryTest, ThrowsForMissingSymbol) {
    const DynamicLibrary library(LAB4_BASIC_PATH);
    EXPECT_THROW(library.symbol("isPrime"), std::runtime_error);
    EXPECT_THROW(library.symbol("PrimeCount"), std::runtime_error);
}

TEST(DynamicLibraryTest, MoveKeepsHandle) {
    DynamicLibrary first(LAB4_BASIC_PATH);
    DynamicLibrary second(std::move(first));
    EXPECT_EQ(second.path(), LAB4_BASIC_PATH);
    EXPECT_NE(second.symbol("primeCount"), nullptr);
    DynamicLibrary third(LAB4_ADVANCED_PATH);
    third = std::move(second);
    EXPECT_EQ(third.path(), LAB4_BASIC_PATH);
    EXPECT_NE(third.symbol("pi"), nullptr);
}

TEST(DynamicBackendTest, RequiresAtLeastOneLibrary) {
    EXPECT_THROW(DynamicBackend({}), std::invalid_argument);
}

TEST(DynamicBackendTest, TogglesBetweenLibraries) {
    DynamicBackend backend({LAB4_BASIC_PATH, LAB4_ADVANCED_PATH});
    EXPECT_EQ(backend.current(), LAB4_BASIC_PATH);
    EXPECT_FLOAT_EQ(backend.pi(1), 4.0F);
    EXPECT_EQ(backend.toggle(), std::string("Switched to ") + LAB4_ADVANCED_PATH);
    EXPECT_NEAR(backend.pi(1), 8.0F / 3.0F, 1e-6F);
    EXPECT_EQ(backend.primeCount(1, 10), 4);
    backend.toggle();
    EXPECT_EQ(backend.current(), LAB4_BASIC_PATH);
}

TEST(DynamicBackendTest, FailedToggleKeepsCurrentLibrary) {
    DynamicBackend backend({LAB4_BASIC_PATH, "libdoes-not-exist.so"});
    EXPECT_THROW(backend.toggle(), std::runtime_error);
    EXPECT_EQ(backend.current(), LAB4_BASIC_PATH);
    EXPECT_EQ(backend.primeCount(1, 10), 4);
}
