#pragma once

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <limits.h> // For INT_MAX
#include <functional>
#include <optional>

#include "state.hpp" // Needs full State definition
#include "point2d.hpp" // For Point2D
#include "agent.hpp"   // For Agent
#include "goal.hpp"    // For Goal
#include "level.hpp"   // For Level::walls, Level::goalsMap

// Forward declaration if needed, but State is included fully
// class State;

namespace HeuristicCalculators {

class HeuristicCalculator {
public:
    static const int MAX_HEURISTIC_VALUE = 1000000; // A large enough value to represent infinity

    HeuristicCalculator(Level& level) : currentLevel_(level) {}
    virtual ~HeuristicCalculator() = default;
    virtual int calculateH(const State& state) = 0;
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
    Level& currentLevel_;
};

class ZeroHeuristicCalculator : public HeuristicCalculator {
public:
    ZeroHeuristicCalculator(Level& level) : HeuristicCalculator(level) {}
    int calculateH(const State& state) override;
    std::string getName() const override;
};

class ManhattanDistanceCalculator : public HeuristicCalculator {
public:
    ManhattanDistanceCalculator(Level& level) : HeuristicCalculator(level) {}
    int calculateH(const State& state) override;
    std::string getName() const override;
};

class SumOfIndividualCostsCalculator : public HeuristicCalculator {
public:
    SumOfIndividualCostsCalculator(Level& level) : HeuristicCalculator(level) {}
    int calculateH(const State& state) override;
    std::string getName() const override { return "sic"; }

    // New method for SCP: calculates path for a single agent to its goal
    // using the provided action_cost_fn.
    // Returns pair: <heuristic_cost, path_actions>
    virtual std::pair<int, std::vector<const Action*>> calculateSingleAgentPath(
        const State& state,
        char agent_id,
        const std::function<int(ActionType)>& action_cost_fn,
        const Level& level_ref // Pass level by const ref for safety
    );

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
    BoxGoalManhattanDistanceCalculator(Level& level) : HeuristicCalculator(level) {}
    int calculateH(const State& state) override;
    std::string getName() const override { return "bgm"; }

    // New method for SCP (simplified): calculates Manhattan distance for a single box to its goal.
    // Does not involve action costs or paths in this version.
    virtual int calculateSingleBoxManhattanDistance(
        const State& state,
        char box_id,
        const Level& level_ref // Pass level by const ref for safety
    );
};

} // namespace HeuristicCalculators 