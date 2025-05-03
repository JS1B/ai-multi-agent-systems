#pragma once

#include <cmath>
#include <string>

#include "state.hpp"

// Forward declaration if State includes Heuristic or for pointer usage,
// but full definition is needed for f() signature.
// class State;

class Heuristic {
   public:
    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    // Make it const as it shouldn't modify the heuristic object itself
    virtual int f(const State& state) const = 0;

    // Calculates the heuristic estimate h(n)
    // Default implementation returns 0, derived classes can override.
    virtual int h(const State& state) const {
        // Basic Manhattan distance example (assuming single agent/goal for simplicity)
        // Needs proper implementation based on actual goals and boxes.
        // This is just a placeholder.
        int estimate = 0;
        // Example: distance for agent 0 to its goal if defined
        // This needs the actual goal coordinates for agent 0
        // for (size_t r = 0; r < state.goals.size(); ++r) {
        //     for (size_t c = 0; c < state.goals[r].size(); ++c) {
        //         if (state.goals[r][c] == '0') { // Assuming goal for agent 0 is '0'
        //             estimate += std::abs(state.agentRows[0] - static_cast<int>(r));
        //             estimate += std::abs(state.agentCols[0] - static_cast<int>(c));
        //             goto goal_found; // Found agent 0 goal
        //         }
        //     }
        // }
        // goal_found:

        // Add distances for boxes to their goals
        // Needs mapping from box char to goal char/location

        return estimate;  // Placeholder
    }

    // Returns the name of the heuristic strategy
    // Make it const as it shouldn't modify the heuristic object itself
    virtual std::string getName() const = 0;
};

class HeuristicAStar : public Heuristic {
   public:
    // Constructor doesn't need to call base with state
    HeuristicAStar(const State& initial_state) {
        // Use initial_state for pre-processing if needed
        (void)initial_state;  // Avoid unused variable warning if not used
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "A*"; }
};

class HeuristicWeightedAStar : public Heuristic {
   public:
    // Constructor doesn't need to call base with state
    HeuristicWeightedAStar(const State& initial_state, int w) : w_(w) {
        // Use initial_state for pre-processing if needed
        (void)initial_state;  // Avoid unused variable warning if not used
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + w_ * h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "WA*(" + std::to_string(w_) + ")"; }

   private:
    int w_;
};

class HeuristicGreedy : public Heuristic {
   public:
    // Constructor doesn't need to call base with state
    HeuristicGreedy(const State& initial_state) {
        // Use initial_state for pre-processing if needed
        (void)initial_state;  // Avoid unused variable warning if not used
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "Greedy"; }
};
