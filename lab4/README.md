# Lab 4. Static and Dynamic Libraries

## Task

Two libraries implement the same contract from `include/contract.hpp`:

- `primeCount(a, b)` counts the primes $p$ with $a \le p \le b$;
- `pi(k)` approximates $\pi$ with $k$ terms.

The first library (`libbasic`) uses trial division and the Leibniz series, the second (`libadvanced`) uses the sieve of Eratosthenes and the Wallis product:

$$
\pi \approx 4 \sum_{n=0}^{k-1} \frac{(-1)^n}{2n + 1}, \qquad
\pi \approx 2 \prod_{n=1}^{k} \frac{4n^2}{4n^2 - 1}
$$

`lab4_static` is linked with the first implementation at build time.
`lab4_dynamic` loads `libbasic.so` at run time and switches between the libraries with command `0`.

## Build and run

```bash
make run LAB=lab4 APP=dynamic < lab4/tests/data/dynamic/01.in
```

Commands: `1 A B` counts primes, `2 K` computes pi, `0` switches the library, `q` or end of input quits.

## Example

Input:

```text
1 1 10
2 1000
0
1 1 10
2 1000
1 -5 20
0
2 1000
```

Output of `lab4_dynamic`:

```text
Loaded libbasic.so
PrimeCount = 4
Pi = 3.14059
Switched to libadvanced.so
PrimeCount = 4
Pi = 3.14081
PrimeCount = 8
Switched to libbasic.so
Pi = 3.14059
```

## Notes

- Both series converge slowly: after $k$ terms the Leibniz error is about $\frac{1}{k}$ and the Wallis error is about $\frac{\pi}{4k}$, which matches $3.14059$ and $3.14081$ for $k = 1000$.
- The contract functions were renamed from `PrimeCount`/`Pi` to `primeCount`/`pi` to follow the naming rules.
- `lab4_dynamic` has `RUNPATH $ORIGIN` and resolves bare library names next to its executable, so it works from any directory and under sanitizers.
- `make trace LAB=lab4` regenerates `docs/strace-static.log` and `docs/strace-dynamic.log`.
