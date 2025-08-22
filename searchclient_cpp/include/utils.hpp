#pragma once

#include <string>
#include <vector>

#include "action.hpp"

namespace utils {
extern std::string whitespaces;

std::string join(const std::vector<std::string> &strings, const std::string &separator);
std::string trim(const std::string &str);
std::string normalizeWhitespace(const std::string &str);
std::string toLower(const std::string &str);

size_t SIC(const std::vector<std::vector<std::vector<const Action *>>> &solutions);

template <typename T>
std::vector<std::vector<T>> transpose(const std::vector<std::vector<T>> &vector2d) {
    std::vector<std::vector<T>> result(vector2d[0].size(), std::vector<T>(vector2d.size()));
    for (size_t i = 0; i < vector2d.size(); ++i) {
        for (size_t j = 0; j < vector2d[0].size(); ++j) {
            result[j][i] = vector2d[i][j];
        }
    }
    return result;
}

template <class T>
void hashCombine(std::size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // namespace utils
