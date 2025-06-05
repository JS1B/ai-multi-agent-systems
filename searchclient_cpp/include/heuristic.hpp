#pragma once

#include <cmath>
#include <string>

#include "low_level_state.hpp"

class Heuristic {
   public:
    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    virtual int f(const LowLevelState& state) const = 0;

    // Calculates the heuristic estimate h(n)
    virtual int h(const LowLevelState& state) const {
        (void)state;
        return 0;
    }
    virtual std::string getName() const = 0;
};

class HeuristicAStar : public Heuristic {
   public:
    HeuristicAStar() {}

    int f(const LowLevelState& state) const override { return state.getG() + h(state); }

    int h(const LowLevelState& state) const override {
        int total_distance = 0;

        // Calculate Manhattan distance for agents to their goals
        for (size_t i = 0; i < state.agent_bulk.size(); i++) {
            Cell2D agent_pos = state.agent_bulk.getPosition(i);
            Cell2D agent_goal = state.agent_bulk.getGoal(i);
            total_distance += std::abs(agent_pos.r - agent_goal.r) + std::abs(agent_pos.c - agent_goal.c);
        }

        return total_distance;
    }

    std::string getName() const override { return "A*"; }
};
