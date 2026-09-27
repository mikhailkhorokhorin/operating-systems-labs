#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

using Matrix = std::vector<std::vector<double>>;

class SingularMatrixError : public std::runtime_error {
public:
    SingularMatrixError();
};

std::vector<double> solveLinearSystem(const Matrix& matrix, const std::vector<double>& rhs,
                                      std::size_t threadCount);
