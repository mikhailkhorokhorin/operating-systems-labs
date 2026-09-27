#include "gauss_jordan.hpp"

#include <algorithm>
#include <barrier>
#include <cmath>
#include <thread>
#include <utility>

namespace {

constexpr double RELATIVE_TOLERANCE = 1e-12;

class ParallelSolver {
public:
    ParallelSolver(const Matrix& matrix, const std::vector<double>& rhs, std::size_t threadCount)
        : size_(matrix.size()),
          threadCount_(std::clamp<std::size_t>(threadCount, 1, std::max<std::size_t>(size_, 1))),
          rows_(buildAugmented(matrix, rhs)),
          tolerance_(computeScale(matrix) * RELATIVE_TOLERANCE),
          barrier_(static_cast<std::ptrdiff_t>(threadCount_), PivotStep{this}) {}

    std::vector<double> solve() {
        {
            std::vector<std::jthread> workers;
            workers.reserve(threadCount_);
            for (std::size_t id = 0; id < threadCount_; ++id) {
                workers.emplace_back([this, id] { work(id); });
            }
        }
        if (singular_) {
            throw SingularMatrixError();
        }
        std::vector<double> solution(size_);
        for (std::size_t row = 0; row < size_; ++row) {
            solution[row] = rows_[row][size_];
        }
        return solution;
    }

private:
    struct PivotStep {
        ParallelSolver* solver;

        void operator()() const noexcept { solver->selectPivot(); }
    };

    static Matrix buildAugmented(const Matrix& matrix, const std::vector<double>& rhs) {
        Matrix rows;
        rows.reserve(matrix.size());
        for (std::size_t row = 0; row < matrix.size(); ++row) {
            auto& augmented = rows.emplace_back(matrix[row]);
            augmented.push_back(rhs[row]);
        }
        return rows;
    }

    static double computeScale(const Matrix& matrix) {
        double scale = 0.0;
        for (const auto& row : matrix) {
            for (const double value : row) {
                scale = std::max(scale, std::abs(value));
            }
        }
        return scale;
    }

    void selectPivot() noexcept {
        if (nextColumn_ == size_) {
            finished_ = true;
            return;
        }
        const std::size_t column = nextColumn_++;
        std::size_t best = column;
        for (std::size_t row = column + 1; row < size_; ++row) {
            if (std::abs(rows_[row][column]) > std::abs(rows_[best][column])) {
                best = row;
            }
        }
        const double pivot = rows_[best][column];
        if (std::abs(pivot) <= tolerance_ || !std::isfinite(pivot)) {
            singular_ = true;
            finished_ = true;
            return;
        }
        std::swap(rows_[column], rows_[best]);
        auto& pivotRow = rows_[column];
        for (std::size_t col = column; col <= size_; ++col) {
            pivotRow[col] /= pivot;
        }
        pivotRow[column] = 1.0;
        currentColumn_ = column;
    }

    void eliminate(std::size_t row) {
        const std::size_t column = currentColumn_;
        auto& target = rows_[row];
        const double factor = target[column];
        if (factor == 0.0) {
            return;
        }
        const auto& pivotRow = rows_[column];
        for (std::size_t col = column; col <= size_; ++col) {
            target[col] -= factor * pivotRow[col];
        }
        target[column] = 0.0;
    }

    void work(std::size_t id) {
        while (true) {
            barrier_.arrive_and_wait();
            if (finished_) {
                return;
            }
            for (std::size_t row = id; row < size_; row += threadCount_) {
                if (row != currentColumn_) {
                    eliminate(row);
                }
            }
        }
    }

    std::size_t size_;
    std::size_t threadCount_;
    Matrix rows_;
    double tolerance_;
    std::size_t nextColumn_ = 0;
    std::size_t currentColumn_ = 0;
    bool finished_ = false;
    bool singular_ = false;
    std::barrier<PivotStep> barrier_;
};

}

SingularMatrixError::SingularMatrixError() : std::runtime_error("matrix is singular") {
}

std::vector<double> solveLinearSystem(const Matrix& matrix, const std::vector<double>& rhs,
                                      std::size_t threadCount) {
    if (threadCount == 0) {
        throw std::invalid_argument("thread count must be positive");
    }
    if (rhs.size() != matrix.size()) {
        throw std::invalid_argument("right-hand side size does not match the matrix");
    }
    for (const auto& row : matrix) {
        if (row.size() != matrix.size()) {
            throw std::invalid_argument("matrix must be square");
        }
    }
    if (matrix.empty()) {
        return {};
    }
    ParallelSolver solver(matrix, rhs, threadCount);
    return solver.solve();
}
