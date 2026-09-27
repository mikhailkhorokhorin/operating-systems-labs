#include "line_sum.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

TEST(SumLineTest, SumsWhitespaceSeparatedNumbers) {
    EXPECT_DOUBLE_EQ(sumLine("1.0 4.5"), 5.5);
    EXPECT_DOUBLE_EQ(sumLine("  -2\t3.25   10 "), 11.25);
}

TEST(SumLineTest, EmptyLineIsZero) {
    EXPECT_DOUBLE_EQ(sumLine(""), 0.0);
    EXPECT_DOUBLE_EQ(sumLine(" \t\r"), 0.0);
}

TEST(SumLineTest, AcceptsSignsAndExponents) {
    EXPECT_DOUBLE_EQ(sumLine("+1e3 -1"), 999.0);
}

TEST(SumLineTest, KeepsDoublePrecision) {
    EXPECT_DOUBLE_EQ(sumLine("16777217 1"), 16777218.0);
}

TEST(SumLineTest, IgnoresCarriageReturn) {
    EXPECT_DOUBLE_EQ(sumLine("7 8\r"), 15.0);
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
    EXPECT_THROW(sumLine("1.5x"), std::invalid_argument);
    EXPECT_THROW(sumLine("+"), std::invalid_argument);
}

TEST(FormatNumberTest, UsesShortestReadableForm) {
    EXPECT_EQ(formatNumber(5.5), "5.5");
    EXPECT_EQ(formatNumber(0.1 + 0.2), "0.3");
    EXPECT_EQ(formatNumber(-0.0), "0");
    EXPECT_EQ(formatNumber(16777218.0), "16777218");
}

TEST(SumStreamTest, WritesOneSumPerLine) {
    std::istringstream in("1 2\n\n3.5 -0.5\n");
    std::ostringstream out;
    sumStream(in, out);
    EXPECT_EQ(out.str(), "3\n0\n3\n");
}

TEST(SumStreamTest, HandlesMissingTrailingNewline) {
    std::istringstream in("1 2");
    std::ostringstream out;
    sumStream(in, out);
    EXPECT_EQ(out.str(), "3\n");
}

TEST(SumStreamTest, ReportsLineOfInvalidNumber) {
    std::istringstream in("1\n2 x\n");
    std::ostringstream out;
    try {
        sumStream(in, out);
        FAIL() << "expected an exception";
    } catch (const std::invalid_argument& error) {
        EXPECT_STREQ(error.what(), "line 2: invalid number 'x'");
    }
    EXPECT_EQ(out.str(), "1\n");
}
