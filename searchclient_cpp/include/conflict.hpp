#pragma once

#include "constraint.hpp"

class Conflict {
   public:
    Conflict(char a1_symbol, char a2_symbol, Constraint constraint) : a1_symbol(a1_symbol), a2_symbol(a2_symbol), constraint(constraint) {}
    ~Conflict() = default;

    char a1_symbol;
    char a2_symbol;
    Constraint constraint;

    bool operator==(const Conflict &other) const {
        return a1_symbol == other.a1_symbol && a2_symbol == other.a2_symbol && constraint == other.constraint;
    }
};