# Lab 3. Shared Memory Between Processes

## Task

Same as [lab 1](../lab1/), but the processes exchange data through a POSIX shared memory segment instead of a pipe.
The parent reads a file name, copies the file into the segment and starts the child.
Variant: the child replaces the content with the sum of the numbers of every line, the parent prints the result.

## Build and run

```bash
echo lab3/tests/data/01.in | make run LAB=lab3
```

## Example

Input file `lab3/tests/data/01.in`:

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

- The segment is named `/lab3-<pid>-<n>`, created with `O_EXCL` and removed by an RAII wrapper.
- The segment starts with a 64-bit length header, so any file size is supported; the child resizes the segment for its answer.
- The child receives the segment name as `argv[1]` and is located next to the parent executable.
- Sums are computed in `double`. An invalid token, `inf`, `nan` or a sum that overflows `double` stops the child with exit code 1.
- `make trace LAB=lab3` regenerates `docs/strace.log`.
