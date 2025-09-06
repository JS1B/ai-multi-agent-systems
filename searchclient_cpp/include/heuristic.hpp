#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>

#include "low_level_state.hpp"

class Heuristic {
   public:
    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    virtual size_t f(const LowLevelState& state) const = 0;

    // Calculates the heuristic estimate h(n)
    virtual size_t h(const LowLevelState& state) const {
        (void)state;
        return 0;
    }
    virtual std::string getName() const = 0;
};

class HeuristicAStar : public Heuristic {
   private:
    // mutable std::unordered_map<size_t, int> heuristic_cache_;

    static constexpr int MOVE_COST = 2;
    static constexpr int PUSH_PULL_COST = 3;    // Push/Pull actions are more expensive
    static constexpr int DEADLOCK_PENALTY = 0;  // Disabled completely for testing

    size_t manhattanDistance(const Cell2D& from, const Cell2D& to) const { return std::abs(from.r - to.r) + std::abs(from.c - to.c); }

    // Detect simple deadlock patterns (box in corner with no goal)
    bool isDeadlock(const Cell2D& box_pos, const LowLevelState& state) const {
        // Check if box is in a corner with no escape
        // This is a simplified deadlock detection
        int_fast8_t blocked_directions = 0;

        Cell2D neighbors[] = {
            {box_pos.r - 1, box_pos.c}, {box_pos.r + 1, box_pos.c}, {box_pos.r, box_pos.c - 1}, {box_pos.r, box_pos.c + 1}};

        for (const auto& neighbor : neighbors) {
            if (!state.getStaticLevel().isCellFree(neighbor) || state.getBoxAt(neighbor)) {
                blocked_directions++;
            }
        }

        // Only consider true corner deadlocks (all 4 directions blocked)
        return blocked_directions >= 4;
    }

    // Calculate cost for agent to manipulate a specific box to goal
    size_t boxManipulationCost(const Cell2D& agent_pos, const Cell2D& box_pos, const Cell2D& goal_pos, const LowLevelState& state) const {
        // Cost = Agent reaching box + Box manipulation to goal
        size_t agent_to_box = manhattanDistance(agent_pos, box_pos);
        size_t box_to_goal = manhattanDistance(box_pos, goal_pos) * PUSH_PULL_COST;

        size_t deadlock_penalty = isDeadlock(box_pos, state) * DEADLOCK_PENALTY;

        return agent_to_box + box_to_goal + deadlock_penalty;
    }

    // Efficient box-goal assignment with complexity control
    size_t optimizeBoxGoalAssignment(const std::vector<Cell2D>& boxes, const std::vector<Cell2D>& goals, const Cell2D& agent_pos,
                                     const LowLevelState& state) const {
        if (boxes.empty() || goals.empty()) return 0;

        // For very complex cases, use simplified heuristic
        if (boxes.size() > 3 || goals.size() > 3) {
            size_t total_cost = 0;
            for (size_t i = 0; i < boxes.size() && i < goals.size(); i++) {
                // Just use closest goal for each box (much faster)
                size_t min_cost = SIZE_MAX;
                for (const auto& goal : goals) {
                    size_t cost = manhattanDistance(boxes[i], goal) * PUSH_PULL_COST;
                    min_cost = std::min(min_cost, cost);
                }
                total_cost += min_cost;
            }
            return total_cost;
        }

        // For small cases, use optimal assignment
        size_t min_cost = SIZE_MAX;
        std::vector<size_t> goal_indices(goals.size());
        std::iota(goal_indices.begin(), goal_indices.end(), 0);

        do {
            size_t total_cost = 0;
            for (size_t i = 0; i < boxes.size() && i < goals.size(); i++) {
                total_cost += boxManipulationCost(agent_pos, boxes[i], goals[goal_indices[i]], state);
            }
            min_cost = std::min(min_cost, total_cost);
        } while (std::next_permutation(goal_indices.begin(), goal_indices.end()));

        return min_cost;
    }

   public:
    HeuristicAStar() {}

    size_t f(const LowLevelState& state) const override { return state.getG() + h(state); }

    size_t h(const LowLevelState& state) const override {
        // Ultra-simple Manhattan distance heuristic - no caching, no complexity
        size_t total_cost = 0;

        // Agent distances to their goals
        for (const auto& agent : state.agents) {
            const auto& agent_goals = agent.getGoalPositions();
            if (!agent_goals.empty()) {
                size_t min_dist = SIZE_MAX;
                for (const auto& goal : agent_goals) {
                    min_dist = std::min(min_dist, manhattanDistance(agent.getPosition(), goal));
                }
                total_cost += min_dist;
            }
        }

        // Box distances to their goals
        for (const auto& box_bulk : state.getBoxBulks()) {
            if (box_bulk.getGoalsCount() > 0) {
                for (size_t i = 0; i < box_bulk.size(); i++) {
                    Cell2D box_pos = box_bulk.getPosition(i);
                    size_t min_dist = SIZE_MAX;
                    for (size_t j = 0; j < box_bulk.getGoalsCount(); j++) {
                        min_dist = std::min(min_dist, manhattanDistance(box_pos, box_bulk.getGoal(j)));
                    }
                    if (min_dist != SIZE_MAX) {
                        total_cost += min_dist;
                    }
                }
            }
        }

        return total_cost;
    }

    std::string getName() const override { return "Modified A*"; }
};
