#pragma once

#include <cmath>
#include <string>
#include <memory> // For std::unique_ptr

#include "state.hpp"
#include "HeuristicCalculator.hpp" // Include the new calculator interface

// Forward declaration if State includes Heuristic or for pointer usage,
// but full definition is needed for f() signature.
// class State;

class Heuristic {
   public:
    // Constructor now takes a HeuristicCalculator
    Heuristic(std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calculator)
        : calculator_(std::move(calculator)) {}

    virtual ~Heuristic() = default;

    // Calculates the heuristic value f(n) = g(n) + h(n)
    // g(n) is the cost to reach state n (usually state.getG())
    // h(n) is the estimated cost from n to goal
    // Make it const as it shouldn't modify the heuristic object itself
    virtual int f(const State& state) const = 0;

    // h(n) is now primarily delegated to the calculator
    virtual int h(const State& state) const {
        if (calculator_) {
            // fprintf(stderr, "DEBUG_HEURISTIC: h() called, calculator_ is valid. Name: %s\n", calculator_->getName().c_str());
            return calculator_->calculateH(state);
        } else {
            // fprintf(stderr, "ERROR_HEURISTIC: h() called, but calculator_ is NULL!\n");
        }
        return 0; 
    }

    // Returns the name of the heuristic strategy
    // Make it const as it shouldn't modify the heuristic object itself
    virtual std::string getName() const = 0;

    // Helper to get the name of the underlying calculator strategy
    std::string getCalculatorName() const {
        if (calculator_) {
            return calculator_->getName();
        }
        return "None";
    }

   protected: // Or private, depending on if derived classes need direct access (they shouldn't normally)
    std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calculator_;
};

class HeuristicAStar : public Heuristic {
   public:
    // Constructor now takes a HeuristicCalculator
    HeuristicAStar(const State& initial_state, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc)
        : Heuristic(std::move(calc)) {
        (void)initial_state; // initial_state can be used by calculator if it needs pre-computation
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "A* (h=" + getCalculatorName() + ")"; }
};

class HeuristicWeightedAStar : public Heuristic {
   public:
    // Constructor now takes a HeuristicCalculator
    HeuristicWeightedAStar(const State& initial_state, int w, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc)
        : Heuristic(std::move(calc)), w_(w) {
        (void)initial_state;
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return state.getG() + w_ * h(state); }

    // Implement getName, matching base signature
    std::string getName() const override { return "WA*(" + std::to_string(w_) + ", h=" + getCalculatorName() + ")"; }

   private:
    int w_;
};

class HeuristicGreedy : public Heuristic {
   public:
    // Constructor now takes a HeuristicCalculator
    HeuristicGreedy(const State& initial_state, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc)
        : Heuristic(std::move(calc)) {
        (void)initial_state;
    }

    // Match base signature: const State&, const
    int f(const State& state) const override { return h(state); } // g(n) is 0 for Greedy

    // Implement getName, matching base signature
    std::string getName() const override { return "Greedy (h=" + getCalculatorName() + ")"; }
};
