#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace utils {
extern std::string whitespaces;

inline std::string join(const std::vector<std::string> &strings, const std::string &separator) {
    std::string result;
    for (const auto &str : strings) {
        result += str;
        if (str != strings.back()) {
            result += separator;
        }
    }
    return result;
}

inline std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(whitespaces);
    if (std::string::npos == first) {
        return str;  // String is all whitespace
    }
    size_t last = str.find_last_not_of(whitespaces);
    return str.substr(first, (last - first + 1));
}

inline std::string normalizeWhitespace(const std::string &str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(c)) {
            result += c;
        }
    }
    return result;
}

inline std::string toLower(const std::string &str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

template <class T>
inline void hashCombine(std::size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // namespace utils
