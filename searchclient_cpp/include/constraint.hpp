#pragma once

#include <cstddef>

#include "cell2d.hpp"

class Constraint {
   public:
    Constraint(Cell2D vertex, size_t g) : vertex(vertex), g(g) {}
    ~Constraint() = default;

    Cell2D vertex;
    size_t g;
};