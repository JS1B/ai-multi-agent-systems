#pragma once

#include <cmath>
#include <string>

#include "state.hpp"

class Heuristic {
   public:
    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    virtual int f(const State& state) const = 0;

    // Calculates the heuristic estimate h(n)
    virtual int h(const State& state) const { return 0; }
    virtual std::string getName() const = 0;
};

class HeuristicAStar : public Heuristic {
   public:
    HeuristicAStar() {}

    int f(const State& state) const override { return state.getG() + h(state); }

    int h(const State& state) const override {
        int total_distance = 0;

        // Calculate Manhattan distance for agents to their goals
        for (size_t agent_idx = 0; agent_idx < state.level.agents.size(); ++agent_idx) {
            if (state.level.agent_goals[agent_idx].r != 0) {  // Skip agents without goals
                total_distance += std::abs(state.level.agents[agent_idx].r - state.level.agent_goals[agent_idx].r) +
                                  std::abs(state.level.agents[agent_idx].c - state.level.agent_goals[agent_idx].c);
            }
        }

        // Calculate Manhattan distance for boxes to their goals
        for (size_t i = 0; i < state.level.boxes.data.size(); ++i) {
            if (!(state.level.boxes.data[i] && state.level.box_goals.data[i])) {
                continue;
            }

            // Find the box's current position
            int box_row = i / state.level.walls.cols;
            int box_col = i % state.level.walls.cols;

            // Find the box's goal position
            int goal_row = 0, goal_col = 0;
            for (size_t j = 0; j < state.level.box_goals.data.size(); ++j) {
                if (state.level.box_goals.data[j] == state.level.boxes.data[i]) {
                    goal_row = j / state.level.walls.cols;
                    goal_col = j % state.level.walls.cols;
                    break;
                }
            }

            total_distance += std::abs(box_row - goal_row) + std::abs(box_col - goal_col);
        }

        return total_distance;
    }

    std::string getName() const override { return "A*"; }
};
