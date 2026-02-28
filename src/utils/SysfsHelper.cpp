#include "SysfsHelper.h"
#include <fstream>
#include <sstream>
#include <glob.h>

namespace SysfsHelper {

std::optional<std::string> readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    if (!content.empty() && content.back() == '\n')
        content.pop_back();
    return content;
}

bool writeFile(const std::string& path, const std::string& value) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << value;
    return f.good();
}

std::optional<long> readLong(const std::string& path) {
    auto s = readFile(path);
    if (!s) return std::nullopt;
    try { return std::stol(*s); }
    catch (...) { return std::nullopt; }
}

bool writeLong(const std::string& path, long value) {
    return writeFile(path, std::to_string(value));
}

std::vector<std::string> glob(const std::string& pattern) {
    glob_t result;
    std::vector<std::string> paths;
    if (::glob(pattern.c_str(), GLOB_TILDE, nullptr, &result) == 0) {
        for (size_t i = 0; i < result.gl_pathc; ++i)
            paths.push_back(result.gl_pathv[i]);
    }
    globfree(&result);
    return paths;
}

} // namespace SysfsHelper
