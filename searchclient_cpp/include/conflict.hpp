#pragma once

#include <utility>

#include "constraint.hpp"

class OneSidedConflict {
   public:
    OneSidedConflict(char a1_symbol, Constraint constraint) : a1_symbol(a1_symbol), constraint(constraint) {}
    ~OneSidedConflict() = default;

    char a1_symbol;
    Constraint constraint;

    bool operator<(const OneSidedConflict &other) const {
        return a1_symbol < other.a1_symbol || (a1_symbol == other.a1_symbol && constraint < other.constraint);
    }
    bool operator==(const OneSidedConflict &other) const { return a1_symbol == other.a1_symbol && constraint == other.constraint; }
};

class FullConflict {
   public:
    FullConflict(char a1_symbol, char a2_symbol, Constraint constraint)
        : a1_symbol(a1_symbol), a2_symbol(a2_symbol), constraint(constraint) {}
    ~FullConflict() = default;

    char a1_symbol;
    char a2_symbol;
    Constraint constraint;

    std::pair<OneSidedConflict, OneSidedConflict> split() const {
        return {OneSidedConflict(a1_symbol, constraint), OneSidedConflict(a2_symbol, constraint)};
    }

    bool operator==(const FullConflict &other) const {
        return a1_symbol == other.a1_symbol && a2_symbol == other.a2_symbol && constraint == other.constraint;
    }
};
