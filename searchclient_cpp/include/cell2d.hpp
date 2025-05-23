#pragma once

#include <cstdint>

// TODO: use int_fast8_t instead of int if performance is an issue

struct Cell2D {
    //int r, c; // row and column
    int_fast8_t r, c; // row and column

    Cell2D() = default;
    Cell2D(int row, int col) : r(row), c(col) {}

    inline bool operator==(const Cell2D& o) const { return r == o.r && c == o.c; }
    inline bool operator!=(const Cell2D& o) const { return r != o.r || c != o.c; }

    inline Cell2D& operator+=(const Cell2D& o) { r += o.r; c += o.c; return *this; }
    inline Cell2D& operator-=(const Cell2D& o) { r -= o.r; c -= o.c; return *this; }

    inline Cell2D operator+(const Cell2D& o) const { return Cell2D(r + o.r, c + o.c); }
    inline Cell2D operator-(const Cell2D& o) const { return Cell2D(r - o.r, c - o.c); }
};
