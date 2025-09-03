#pragma once

#include <cstdint>
#include <vector>

#include "cell2d.hpp"

class Agent {
   private:
    Cell2D position_;
    std::vector<Cell2D> goal_positions_;  // if any, TODO check
    const char symbol_;

   public:
    Agent() = delete;
    Agent(const Cell2D &position, const std::vector<Cell2D> &goals, char symbol)
        : position_(position), goal_positions_(goals), symbol_(symbol) {
        goal_positions_.shrink_to_fit();
    }
    Agent(const Agent &) = default;
    Agent &operator=(const Agent &) = delete;
    ~Agent() = default;

    char getSymbol() const { return symbol_; }
    Cell2D &position() { return position_; }
    const Cell2D &getPosition() const { return position_; }
    const std::vector<Cell2D> &getGoalPositions() const { return goal_positions_; }

    bool operator==(const Agent &other) const {
        return symbol_ == other.symbol_ && position_ == other.position_ && goal_positions_ == other.goal_positions_;
    }

    bool reachedGoal(void) const {
        bool reached = true;
        for (const auto &goal : goal_positions_) {
            if (position_ != goal) {
                reached = false;
                break;
            }
        }
        return reached;
    }

    size_t getHash() const {
        size_t hash = position_.r ^ position_.c ^ int(symbol_);
        for (const auto &goal : goal_positions_) {
            hash = hash ^ goal.r ^ goal.c;
        }
        return hash;
    }
};