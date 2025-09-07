#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

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
    // Store initial agent positions for movement penalty calculation
    std::vector<Cell2D> initial_agent_positions_;

    static constexpr int MOVE_COST = 2;
    static constexpr int PUSH_PULL_COST = 3;              // Push/Pull actions are more expensive
    static constexpr int GOALLESS_MOVEMENT_PENALTY = 10;  // Heavy penalty for goalless agents moving

    size_t manhattanDistance(const Cell2D& from, const Cell2D& to) const { return std::abs(from.r - to.r) + std::abs(from.c - to.c); }

    // Calculate cost for agent to manipulate a specific box to goal
    size_t boxManipulationCost(const Cell2D& agent_pos, const Cell2D& box_pos, const Cell2D& goal_pos, const LowLevelState& state) const {
        (void)state;
        // Cost = Agent reaching box + Box manipulation to goal
        size_t agent_to_box = manhattanDistance(agent_pos, box_pos);
        size_t box_to_goal = manhattanDistance(box_pos, goal_pos) * PUSH_PULL_COST;

        return agent_to_box + box_to_goal;
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

        // For small cases, use a decent assignment
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
    HeuristicAStar() = delete;

    // Constructor that accepts initial state for movement penalty calculation
    HeuristicAStar(const LowLevelState* initial_state) {
        if (initial_state) {
            initial_agent_positions_.reserve(initial_state->agents.size());
            for (const auto& agent : initial_state->agents) {
                initial_agent_positions_.push_back(agent.getPosition());
            }
        }
    }

    size_t f(const LowLevelState& state) const override { return state.getG() + h(state); }

    size_t h(const LowLevelState& state) const override {
        // Simple Manhattan distance heuristic
        size_t total_cost = 0;

        // Agent distances to their goals
        for (size_t i = 0; i < state.agents.size(); i++) {
            const auto& agent = state.agents[i];
            const auto& agent_goals = agent.getGoalPositions();

            if (!agent_goals.empty()) {
                // Agent has goals - calculate distance to closest goal
                size_t min_dist = SIZE_MAX;
                for (const auto& goal : agent_goals) {
                    min_dist = std::min(min_dist, manhattanDistance(agent.getPosition(), goal));
                }
                total_cost += min_dist;
            } else if (i < initial_agent_positions_.size()) {
                // Agent has no goals - add heavy penalty for moving from initial position
                size_t movement_penalty = manhattanDistance(agent.getPosition(), initial_agent_positions_[i]) * GOALLESS_MOVEMENT_PENALTY;
                total_cost += movement_penalty;
            }
        }

        // Box distances to their goals - only consider N best boxes where N = number of goals
        for (const auto& box_bulk : state.getBoxBulks()) {
            if (box_bulk.getGoalsCount() <= 0) {
                continue;
            }
            // Collect all boxes with their distances to closest goals
            std::vector<std::pair<size_t, size_t>> box_distances;  // pair<distance, box_index>

            for (size_t i = 0; i < box_bulk.size(); i++) {
                Cell2D box_pos = box_bulk.getPosition(i);
                size_t min_dist = SIZE_MAX;
                for (size_t j = 0; j < box_bulk.getGoalsCount(); j++) {
                    min_dist = std::min(min_dist, manhattanDistance(box_pos, box_bulk.getGoal(j)));
                }
                if (min_dist != SIZE_MAX) {
                    box_distances.push_back({min_dist, i});
                }
            }

            // Sort by distance (shortest first)
            std::sort(box_distances.begin(), box_distances.end());

            std::vector<std::pair<size_t, size_t>> box_agents_distances;
            for (size_t i = 0; i < box_distances.size(); i++) {
                // Add box-to-goal distance plus agent-to-box distance
                Cell2D box_pos = box_bulk.getPosition(box_distances[i].second);
                Cell2D agent_pos = state.agents[0].getPosition();  // Use first agent for simplicity
                size_t agent_to_box_dist = manhattanDistance(agent_pos, box_pos);
                box_agents_distances.push_back({box_distances[i].first + agent_to_box_dist, i});
            }

            // Only consider the N closest boxes where N = number of goals
            std::sort(box_agents_distances.begin(), box_agents_distances.end());

            for (size_t i = 0; i < box_bulk.getGoalsCount(); i++) {
                total_cost += box_agents_distances[i].first;
            }
        }

        return total_cost;
    }

    std::string getName() const override { return "Modified A*"; }
};
