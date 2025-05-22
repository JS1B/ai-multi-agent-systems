#pragma once

#include <cstdlib>
#include <vector>
#include <string_view>
#include <string>

#include "cell2d.hpp"

struct CharGrid {
    size_t rows, cols;
    std::vector<char> data;

    CharGrid(size_t rows, size_t cols) : rows(rows), cols(cols), data(rows * cols, 0) {}
    CharGrid(const Cell2D& size) : rows(size.r), cols(size.c), data(size.r * size.c, 0) {}

    char& operator()(size_t row, size_t col) // [] operator cannot accept 2 arguments, so we use () instead
    { 
        return data[row * cols + col]; 
    }

    const char& operator()(size_t row, size_t col) const
    { 
        return data[row * cols + col]; 
    }

    char& operator()(const Cell2D& cell)
    {
        return data[cell.r * cols + cell.c];
    }

    const char& operator()(const Cell2D& cell) const
    {
        return data[cell.r * cols + cell.c];
    }

    size_t get_hash() const {
        auto sv = std::string_view{ data.data(), data.size() };
        return std::hash<std::string_view>()(sv);
    }

    bool operator==(const CharGrid& other) const { return data == other.data; }
    bool operator!=(const CharGrid& other) const { return data != other.data; }

    size_t size_rows() const { return rows; }
    size_t size_cols() const { return cols; }
    Cell2D size() const { return Cell2D(rows, cols); }

    std::string to_string() const {
        std::string result;
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                char ch = (*this)(r, c);
                result.push_back(ch ? ch : ' '); // If the character is 0, we print a space
            }
            result.push_back('\n');
        }
        return result;
    }
};

namespace std {
    template<>
    struct hash<CharGrid> {
        size_t operator()(CharGrid const& g) const noexcept {
            return g.get_hash();
        }
    };
}
