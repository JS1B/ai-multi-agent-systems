#ifndef HEURISTICCALCULATOR_HPP
#define HEURISTICCALCULATOR_HPP

#pragma once // This is fine, but guards are more traditional for .hpp

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <limits.h> // For INT_MAX
#include <functional>
#include <optional>
#include <unordered_map> // For ActionCostMap's underlying type

#include "state.hpp" // Needs full State definition
#include "point2d.hpp" // For Point2D
#include "agent.hpp"   // For Agent
#include "goal.hpp"    // For Goal
#include "level.hpp"   // For Level::walls, Level::goalsMap
#include "action.hpp" // For ActionType

// Forward declaration if needed, but State is included fully
// class State;

namespace HeuristicCalculators {

// Define ActionCostMap using typedef directly within the namespace
typedef std::unordered_map<ActionType, double> ActionCostMap;

// Forward declaration for HeuristicCalculator itself if needed by subtractCosts, though unlikely.
// class HeuristicCalculator; // Not needed, subtractCosts is standalone or can be static member

// Declaration for subtractCosts helper function
ActionCostMap subtractCosts(const ActionCostMap& main_costs, const ActionCostMap& costs_to_subtract);

class HeuristicCalculator {
public:
    static const int MAX_HEURISTIC_VALUE = 1000000; // A large enough value to represent infinity

    HeuristicCalculator(const Level& level_ref) : level_(level_ref) {}
    virtual ~HeuristicCalculator() = default;

    // Calculates the heuristic value h(n) for a given state using provided action costs
    // Make it const as it shouldn't modify the heuristic object itself
    virtual int calculateH(const State& state, const ActionCostMap& costs) const = 0;

    // Determines the minimal costs this heuristic needs to achieve its h-value for the state,
    // given the currently available costs.
    virtual ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const = 0;

    // Returns the name of the heuristic calculation method (e.g., "MDS", "SIC")
    // Make it const as it shouldn't modify the heuristic object itself
    virtual std::string getName() const = 0;

    // BFS function made public static for use by SCPHeuristicCalculator and others
    // It finds a path for a specific agent (if agent_id_char_opt is provided) or generally between two points.
    // The action_cost_fn determines the cost of each action type.
    public: // Explicitly public, was protected
        static std::pair<int, std::vector<const Action*>> bfs(
            Point2D start_pos,
            Point2D end_pos,
            const State& current_state, // To check for dynamic obstacles like other agents/boxes if needed by action_cost_fn or validity
            const Level& level,         // To check for walls
            std::function<int(ActionType)> action_cost_fn,
            std::optional<char> agent_id_char_opt = std::nullopt // Specify which agent is moving for context
        );

protected:
    const Level& level_; // Reference to the static level data
};

class ZeroHeuristicCalculator : public HeuristicCalculator {
public:
    ZeroHeuristicCalculator(const Level& level_ref) : HeuristicCalculator(level_ref) {}

    int calculateH(const State& state, const ActionCostMap& costs) const override {
        (void)state; // Unused
        (void)costs; // Unused
        return 0;
    }

    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override {
        (void)state; // Unused
        (void)available_costs; // Unused
        // Zero heuristic consumes no costs.
        ActionCostMap consumed_costs;
        // Ensure all action types present in available_costs are also in consumed_costs with 0.0 cost
        // Or, more simply, if an action type is not in consumed_costs, it's assumed to be 0.
        // For explicit zero consumption:
        for(const auto& cost_pair : available_costs){
            consumed_costs[cost_pair.first] = 0.0;
        }
        return consumed_costs; 
    }

    std::string getName() const override { return "Zero"; }
};

class ManhattanDistanceCalculator : public HeuristicCalculator {
public:
    ManhattanDistanceCalculator(const Level& level_ref) : HeuristicCalculator(level_ref) {}
    int calculateH(const State& state, const ActionCostMap& costs) const override;
    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override;
    std::string getName() const override;
};

class SumOfIndividualCostsCalculator : public HeuristicCalculator {
public:
    SumOfIndividualCostsCalculator(const Level& level_ref) : HeuristicCalculator(level_ref) {}
    int calculateH(const State& state, const ActionCostMap& costs) const override;
    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override;
    std::string getName() const override { return "sic"; }

    // New method for SCP: calculates path for a single agent to its goal
    // using the provided action_cost_fn.
    // Returns pair: <heuristic_cost, path_actions>
    virtual std::pair<int, std::vector<const Action*>> calculateSingleAgentPath(
        const State& state,
        char agent_id,
        const std::function<int(ActionType)>& action_cost_fn,
        const Level& level_ref // Pass level by const ref for safety
    ) const;

    // New method for Greedy SCP
    virtual std::pair<int, ActionCostMap> calculate_and_saturate_for_agent(
        char agent_id,
        const State& state,
        const ActionCostMap& current_budget
    ) const;

private:
    // Helper for BFS path calculation for a single agent
    int calculate_single_agent_path_cost(
        const Point2D& start_pos,
        const Point2D& goal_pos,
        int num_rows,
        int num_cols,
        const std::vector<std::vector<bool>>& walls
    ) const;
};

class BoxGoalManhattanDistanceCalculator : public HeuristicCalculator {
public:
    BoxGoalManhattanDistanceCalculator(const Level& level_ref) : HeuristicCalculator(level_ref) {}
    int calculateH(const State& state, const ActionCostMap& costs) const override;
    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override;
    std::string getName() const override { return "bgm"; }

    // New method for SCP (simplified): calculates Manhattan distance for a single box to its goal.
    // Does not involve action costs or paths in this version.
    virtual int calculateSingleBoxManhattanDistance(
        const State& state,
        char box_id,
        const Level& level_ref // Pass level by const ref for safety
    ) const;
};

} // namespace HeuristicCalculators 

#endif // HEURISTICCALCULATOR_HPP 