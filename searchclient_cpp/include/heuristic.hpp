#pragma once

#include <cmath>
#include <string>
#include <vector> // Added
#include <memory> // For std::unique_ptr
#include <algorithm> // For std::max

#include "state.hpp"
#include "HeuristicCalculator.hpp" // Include the new calculator interface

// Forward declaration if State includes Heuristic or for pointer usage,
// but full definition is needed for f() signature.
// class State;

class Heuristic {
   public:
    // Constructor now takes a vector of HeuristicCalculators
    Heuristic(const State& initial_state, std::vector<std::unique_ptr<HeuristicCalculators::HeuristicCalculator>> calculators)
        : calculators_(std::move(calculators)), initial_state_ref_(initial_state) {}

    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    // Make it const as it shouldn't modify the heuristic object itself
    virtual int f(const State& state) const = 0;

    // h(n) is now primarily delegated to the calculator
    virtual int h(const State& state) const {
        if (calculators_.empty()) {
            // fprintf(stderr, "ERROR_HEURISTIC: h() called, but calculators_ vector is empty!\\n");
            return 0; 
        }

        // Create a default ActionCostMap with cost 1.0 for all action types
        HeuristicCalculators::ActionCostMap default_costs;
        for (const Action* action_template : Action::allValues()) { // Assuming Action::allValues() gives templates or types
            if (action_template) { // Should always be true if allValues() is robust
                 default_costs[action_template->type] = 1.0;
            }
        }
        // Ensure all unique action types from Action::allValues() are in the map, even if some share types.
        // The loop above should handle this correctly if Action::allValues() provides representative actions for each type.

        int max_h = 0; 
        bool first = true;
        for (const auto& calculator : calculators_) {
            if (calculator) {
                // Pass the default_costs to the calculator
                int current_h = calculator->calculateH(state, default_costs);
                if (first) {
                    max_h = current_h;
                    first = false;
                } else {
                    max_h = std::max(max_h, current_h);
                }
            }
        }
        return max_h;
    }

    // Returns the name of the heuristic strategy
    // Make it const as it shouldn't modify the heuristic object itself
    virtual std::string getName() const = 0;

    // Helper to get the name of the underlying calculator strategy
    std::string getCalculatorName() const {
        if (calculators_.empty()) {
            return "None";
        }
        if (calculators_.size() == 1 && calculators_[0]) {
            return calculators_[0]->getName();
        }
        std::string name = "MaxOf(";
        bool first = true;
        for (const auto& calculator : calculators_) {
            if (calculator) {
                if (!first) {
                    name += ",";
                }
                name += calculator->getName();
                first = false;
            }
        }
        name += ")";
        return name;
    }

   protected: // Or private, depending on if derived classes need direct access (they shouldn't normally)
    std::vector<std::unique_ptr<HeuristicCalculators::HeuristicCalculator>> calculators_;
    const State& initial_state_ref_;

    // Default costs, initialized if needed. This assumes ActionCostMap is now properly namespaced.
    HeuristicCalculators::ActionCostMap default_costs_;
};

class HeuristicAStar : public Heuristic {
   public:
    // Constructor now takes a vector of HeuristicCalculators
    HeuristicAStar(const State& initial_state, std::vector<std::unique_ptr<HeuristicCalculators::HeuristicCalculator>> calcs)
        : Heuristic(initial_state, std::move(calcs)) {}

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "A* (h=" + getCalculatorName() + ")"; }
};

class HeuristicWeightedAStar : public Heuristic {
   public:
    // Constructor now takes a vector of HeuristicCalculators
    HeuristicWeightedAStar(const State& initial_state, int w, std::vector<std::unique_ptr<HeuristicCalculators::HeuristicCalculator>> calcs)
        : Heuristic(initial_state, std::move(calcs)), w_(w) {}

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + w_ * h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "WA*(" + std::to_string(w_) + ", h=" + getCalculatorName() + ")"; }

   private:
    int w_;
};

class HeuristicGreedy : public Heuristic {
   public:
    // Constructor now takes a vector of HeuristicCalculators
    HeuristicGreedy(const State& initial_state, std::vector<std::unique_ptr<HeuristicCalculators::HeuristicCalculator>> calcs)
        : Heuristic(initial_state, std::move(calcs)) {}

    // Match base signature: const State&, const
    int f(const State& state) const override { return h(state); } // g(n) is 0 for Greedy

    // Implement getName, matching base signature
    std::string getName() const override { return "Greedy (h=" + getCalculatorName() + ")"; }
};
