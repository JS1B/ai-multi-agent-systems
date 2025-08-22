#pragma once

#include <cstddef>

#include "cell2d.hpp"

class Constraint {
   public:
    Constraint() = delete;
    Constraint(Cell2D vertex, size_t g) : vertex(vertex), g(g) {}
    ~Constraint() = default;

    Cell2D vertex;
    size_t g;

    bool operator==(const Constraint &other) const { return vertex == other.vertex && g == other.g; }
};