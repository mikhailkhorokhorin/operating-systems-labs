#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

double sumLine(std::string_view line);

std::string formatNumber(double value);

void sumStream(std::istream& in, std::ostream& out);
