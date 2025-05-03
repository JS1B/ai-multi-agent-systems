#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace utils {
std::string whitespaces = " \t\n\r\f\v";

std::string join(const std::vector<std::string> &strings, const std::string &separator) {
    std::string result;
    for (size_t i = 0; i < strings.size(); i++) {
        result += strings[i];
        if (i < strings.size() - 1) {
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

std::string normalizeWhitespace(const std::string &str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(c)) {
            result += c;
        }
    }
    return result;
}

std::string toLower(const std::string &str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

}  // namespace utils
