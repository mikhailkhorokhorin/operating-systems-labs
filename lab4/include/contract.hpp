#pragma once

using PrimeCountFunc = int (*)(int a, int b);
using PiFunc = float (*)(int k);

extern "C" {
int primeCount(int a, int b);
float pi(int k);
}
