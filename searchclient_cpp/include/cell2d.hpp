#pragma once

#include <cstdint>

// TODO: use int_fast8_t instead of int if performance is an issue

struct Cell2D {
    int r, c; // row and column

    Cell2D() = default;
    Cell2D(int row, int col) : r(row), c(col) {}

    bool operator==(const Cell2D& o) const { return r == o.r && c == o.c; }
    bool operator!=(const Cell2D& o) const { return r != o.r || c != o.c; }

    Cell2D& operator+=(const Cell2D& o) { r += o.r; c += o.c; return *this; }
    Cell2D& operator-=(const Cell2D& o) { r -= o.r; c -= o.c; return *this; }

    Cell2D operator+(const Cell2D& o) const { return Cell2D(r + o.r, c + o.c); }
    Cell2D operator-(const Cell2D& o) const { return Cell2D(r - o.r, c - o.c); }
};
