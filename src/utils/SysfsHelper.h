#pragma once
#include <string>
#include <vector>
#include <optional>

namespace SysfsHelper {
    std::optional<std::string> readFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& value);
    std::optional<long> readLong(const std::string& path);
    bool writeLong(const std::string& path, long value);
    std::vector<std::string> glob(const std::string& pattern);
}
