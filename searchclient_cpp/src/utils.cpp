#include "utils.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

namespace utils {
// Define the extern variable
std::string whitespaces = " \t\n\r\f\v";

std::string join(const std::vector<std::string> &strings, const std::string &separator) {
    std::string result;
    for (const auto &str : strings) {
        result += str;
        if (str != strings.back()) {
            result += separator;
        }
    }
    return result;
}

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(whitespaces);
    if (std::string::npos == first) {
        return str;  // String is all whitespace
    }
    size_t last = str.find_last_not_of(whitespaces);
    return str.substr(first, (last - first + 1));
}

// Replace all whitespaces with a single space
std::string normalizeWhitespace(const std::string &str) {
    std::string result;
    for (char c : str) {
        if (whitespaces.find(c) != std::string::npos) {
            result += ' ';
            continue;
        }
        result += c;
    }
    return result;
}

std::string toLower(const std::string &str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

size_t SIC(const std::vector<std::vector<const Action *>> &solutions) {
    // Sum of individual costs
    return std::accumulate(solutions.begin(), solutions.end(), 0,
                           [](size_t sum, const std::vector<const Action *> &plan) { return sum + plan.size(); });
}

}  // namespace utils