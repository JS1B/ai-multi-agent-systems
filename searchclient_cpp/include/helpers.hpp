#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace utils {
extern std::string whitespaces;

std::string join(const std::vector<std::string> &strings, const std::string &separator);
std::string trim(const std::string &str);
std::string normalizeWhitespace(const std::string &str);
std::string toLower(const std::string &str);

template <class T>
void hashCombine(std::size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // namespace utils
