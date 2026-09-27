# Lab 1. Processes and Pipes

## Task

The parent process reads a file name from standard input, creates a pipe and starts a child process.
The child reads the file through its standard input (redirected with `dup2`) and writes to the pipe.
Variant: every line of the file contains floating-point numbers, the child prints the sum of each line.
The parent prints everything it receives from the pipe and reports a failing child.

## Build and run

```bash
echo lab1/tests/data/01.in | make run LAB=lab1
```

## Example

Input file `lab1/tests/data/01.in`:

```text
1.0 4.5
-2 3.25 10
0.1 0.2

16777217 1
+1e3 -1
```

Output:

```text
5.5
11.25
0.3
0
16777218
999
```

## Notes

- The child (`lab1_child`) is located next to the parent executable via `/proc/self/exe`.
- Sums are computed in `double`, so `16777217 1` gives the exact $16777218$. In `float` the number $2^{24} + 1 = 16777217$ is not representable and the sum would be $16777216$. An empty line sums to 0.
- An invalid token, `inf`, `nan` or a sum that overflows `double` stops the child with an error and exit code 1.
- `make trace LAB=lab1` regenerates `docs/strace.log`.
