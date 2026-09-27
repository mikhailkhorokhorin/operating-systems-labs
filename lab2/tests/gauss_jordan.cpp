#include "gauss_jordan.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

void expectNear(const std::vector<double>& actual, const std::vector<double>& expected,
                double tolerance = 1e-9) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_NEAR(actual[i], expected[i], tolerance) << "index " << i;
    }
}

std::vector<double> multiply(const Matrix& matrix, const std::vector<double>& vector) {
    std::vector<double> result(matrix.size(), 0.0);
    for (std::size_t row = 0; row < matrix.size(); ++row) {
        for (std::size_t col = 0; col < vector.size(); ++col) {
            result[row] += matrix[row][col] * vector[col];
        }
    }
    return result;
}

}

TEST(GaussJordanTest, SolvesSimpleSystem) {
    const Matrix matrix{{2, 1}, {1, 3}};
    expectNear(solveLinearSystem(matrix, {3, 5}, 1), {0.8, 1.4});
}

TEST(GaussJordanTest, SolvesSystemWithZeroOnDiagonal) {
    const Matrix matrix{{0, 2, 1}, {1, 1, 1}, {2, 1, -1}};
    for (std::size_t threads = 1; threads <= 4; ++threads) {
        expectNear(solveLinearSystem(matrix, {7, 6, 1}, threads), {1, 2, 3});
    }
}

TEST(GaussJordanTest, PartialPivotingKeepsAccuracy) {
    const Matrix matrix{{1e-20, 1}, {1, 1}};
    expectNear(solveLinearSystem(matrix, {1, 2}, 2), {1, 1});
}

TEST(GaussJordanTest, DetectsSingularMatrix) {
    const Matrix matrix{{1, 2}, {2, 4}};
    EXPECT_THROW(solveLinearSystem(matrix, {3, 6}, 2), SingularMatrixError);
}

TEST(GaussJordanTest, DetectsZeroMatrix) {
    const Matrix matrix{{0, 0}, {0, 0}};
    EXPECT_THROW(solveLinearSystem(matrix, {0, 0}, 3), SingularMatrixError);
}

TEST(GaussJordanTest, DetectsSingularMatrixLateInElimination) {
    const Matrix matrix{{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 1}, {1, 0, 0, 1}};
    EXPECT_THROW(solveLinearSystem(matrix, {1, 2, 3, 4}, 4), SingularMatrixError);
}

TEST(GaussJordanTest, EmptySystemHasEmptySolution) {
    EXPECT_TRUE(solveLinearSystem({}, {}, 2).empty());
}

TEST(GaussJordanTest, MoreThreadsThanRows) {
    const Matrix matrix{{4}};
    expectNear(solveLinearSystem(matrix, {2}, 16), {0.5});
}

TEST(GaussJordanTest, RejectsZeroThreads) {
    EXPECT_THROW(solveLinearSystem({{1}}, {1}, 0), std::invalid_argument);
}

TEST(GaussJordanTest, RejectsMismatchedDimensions) {
    EXPECT_THROW(solveLinearSystem({{1, 2}, {3, 4}}, {1}, 1), std::invalid_argument);
    EXPECT_THROW(solveLinearSystem({{1, 2}, {3}}, {1, 2}, 1), std::invalid_argument);
}

TEST(GaussJordanTest, InputIsNotModified) {
    const Matrix matrix{{0, 1}, {1, 0}};
    const std::vector<double> rhs{2, 3};
    expectNear(solveLinearSystem(matrix, rhs, 2), {3, 2});
    EXPECT_EQ(matrix, (Matrix{{0, 1}, {1, 0}}));
    EXPECT_EQ(rhs, (std::vector<double>{2, 3}));
}

TEST(GaussJordanTest, ThreadCountDoesNotChangeResult) {
    constexpr std::size_t SIZE = 60;
    std::mt19937 generator(42);
    std::uniform_real_distribution<double> distribution(-10.0, 10.0);
    Matrix matrix(SIZE, std::vector<double>(SIZE));
    std::vector<double> expected(SIZE);
    for (std::size_t row = 0; row < SIZE; ++row) {
        for (double& value : matrix[row]) {
            value = distribution(generator);
        }
        matrix[row][row] += 50.0;
        expected[row] = distribution(generator);
    }
    const auto rhs = multiply(matrix, expected);
    const auto reference = solveLinearSystem(matrix, rhs, 1);
    expectNear(reference, expected, 1e-8);
    for (const std::size_t threads : {2U, 3U, 4U, 8U}) {
        const auto solution = solveLinearSystem(matrix, rhs, threads);
        ASSERT_EQ(solution.size(), reference.size());
        for (std::size_t i = 0; i < SIZE; ++i) {
            EXPECT_DOUBLE_EQ(solution[i], reference[i]) << "threads " << threads;
        }
    }
}
