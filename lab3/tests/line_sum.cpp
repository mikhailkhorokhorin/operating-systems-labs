#include "line_sum.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(SumLineTest, SumsWhitespaceSeparatedNumbers) {
    EXPECT_DOUBLE_EQ(sumLine("1.0 4.5"), 5.5);
    EXPECT_DOUBLE_EQ(sumLine("  -2\t3.25   10 "), 11.25);
}

TEST(SumLineTest, EmptyLineIsZero) {
    EXPECT_DOUBLE_EQ(sumLine(""), 0.0);
    EXPECT_DOUBLE_EQ(sumLine(" \t\r"), 0.0);
}

TEST(SumLineTest, KeepsDoublePrecision) {
    EXPECT_DOUBLE_EQ(sumLine("16777217 1"), 16777218.0);
}

TEST(SumLineTest, RejectsNonFiniteNumbers) {
    EXPECT_THROW(sumLine("inf"), std::invalid_argument);
    EXPECT_THROW(sumLine("1 -infinity"), std::invalid_argument);
    EXPECT_THROW(sumLine("nan"), std::invalid_argument);
    EXPECT_THROW(sumLine("1e400"), std::invalid_argument);
}

TEST(SumLineTest, RejectsOverflowingSum) {
    EXPECT_THROW(sumLine("1.7e308 1.7e308"), std::invalid_argument);
}

TEST(SumLineTest, RejectsInvalidTokens) {
    EXPECT_THROW(sumLine("1 abc"), std::invalid_argument);
    EXPECT_THROW(sumLine("+"), std::invalid_argument);
}

TEST(FormatNumberTest, UsesShortestReadableForm) {
    EXPECT_EQ(formatNumber(0.1 + 0.2), "0.3");
    EXPECT_EQ(formatNumber(-0.0), "0");
}

TEST(SumTextTest, SumsEveryLine) {
    EXPECT_EQ(sumText("1 2\n\n3.5 -0.5\n"), "3\n0\n3\n");
}

TEST(SumTextTest, HandlesMissingTrailingNewline) {
    EXPECT_EQ(sumText("1 2\r\n4"), "3\n4\n");
}

TEST(SumTextTest, EmptyTextGivesEmptyResult) {
    EXPECT_EQ(sumText(""), "");
}

TEST(SumTextTest, ReportsLineOfInvalidNumber) {
    try {
        sumText("1\n2 x\n");
        FAIL() << "expected an exception";
    } catch (const std::invalid_argument& error) {
        EXPECT_STREQ(error.what(), "line 2: invalid number 'x'");
    }
}

TEST(SumTextTest, ResultMayBeLongerThanInput) {
    EXPECT_EQ(sumText("\n\n"), "0\n0\n");
}
