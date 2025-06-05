#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "cell2d.hpp"

struct CharGrid {
    size_t rows, cols;
    std::vector<char> data;

    CharGrid(size_t rows, size_t cols) : rows(rows), cols(cols), data(rows * cols, 0) {}
    CharGrid(const Cell2D& size) : rows(size.r), cols(size.c), data(size.r * size.c, 0) {}

    inline char& operator()(size_t row, size_t col)  // [] operator cannot accept 2 arguments, so we use () instead
    {
        return data[row * cols + col];
    }

    inline const char& operator()(size_t row, size_t col) const { return data[row * cols + col]; }

    inline char& operator()(const Cell2D& cell) { return data[cell.r * cols + cell.c]; }

    inline const char& operator()(const Cell2D& cell) const { return data[cell.r * cols + cell.c]; }

    inline size_t get_hash() const {
        auto sv = std::string_view{data.data(), data.size()};
        return std::hash<std::string_view>()(sv);
    }

    inline bool operator==(const CharGrid& other) const { return data == other.data; }
    inline bool operator!=(const CharGrid& other) const { return data != other.data; }

    inline size_t size_rows() const { return rows; }
    inline size_t size_cols() const { return cols; }
    inline Cell2D size() const { return Cell2D(rows, cols); }

    std::string to_string() const {
        std::string result;
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                char ch = (*this)(r, c);
                result.push_back(ch ? ch : ' ');  // If the character is 0, we print a space
            }
            result.push_back('\n');
        }
        return result;
    }
};

namespace std {
template <>
struct hash<CharGrid> {
    size_t operator()(CharGrid const& g) const noexcept { return g.get_hash(); }
};
}  // namespace std
