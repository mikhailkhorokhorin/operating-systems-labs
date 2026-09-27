#pragma once

#include <iosfwd>
#include <string>

class Backend {
public:
    virtual ~Backend() = default;

    virtual int primeCount(int a, int b) = 0;
    virtual float pi(int k) = 0;
    virtual std::string toggle() = 0;
};

int runShell(std::istream& in, std::ostream& out, std::ostream& err, Backend& backend,
             bool interactive);
