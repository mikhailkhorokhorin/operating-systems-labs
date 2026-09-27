#pragma once

#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>

std::optional<std::string> readFileName(std::istream& in);

int runPipeline(const std::filesystem::path& childPath, const std::filesystem::path& inputPath,
                std::ostream& out, std::ostream& err);

void processSegment(const std::string& segmentName);
