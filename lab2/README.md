# Lab 2. Threads: Parallel Gauss-Jordan Elimination

## Task

Solve a system of linear equations $Ax = b$ with the Gauss-Jordan method using several threads.
The maximum number of threads is passed as a command-line argument (default 2).
Variant: the rows of the matrix are distributed between threads at every elimination step.

## Build and run

```bash
make run LAB=lab2 ARGS=4 < lab2/tests/data/01.in
```

## Example

Input (size, matrix rows, right-hand side):

```text
3
0 2 1
1 1 1
2 1 -1
7 6 1
```

It describes the system

$$
\begin{cases}
2x_1 + x_2 = 7 \\
x_0 + x_1 + x_2 = 6 \\
2x_0 + x_1 - x_2 = 1
\end{cases}
$$

Output:

```text
x[0] = 1
x[1] = 2
x[2] = 3
```

## Notes

- At step $k$ the pivot row is chosen by partial pivoting, $p = \arg\max_{i \ge k} |a_{ik}|$, swapped with row $k$ and divided by $a_{kk}$. Then every other row is updated in parallel, $R_i \leftarrow R_i - a_{ik} R_k$ for $i \ne k$.
- A pool of worker threads is created once. The steps are separated by `std::barrier`, whose completion step selects the pivot.
- The matrix is reported as `error: matrix is singular` when $|a_{pk}| \le 10^{-12} \cdot \max_{i,j} |a_{ij}|$.
- The thread count must be an integer in $[1, 256]$. More threads than rows are not started.
- `make trace LAB=lab2` regenerates `docs/strace.log`.
