#pragma once

#include <string>

#include "state.hpp"

class Heuristic {
   public:
    Heuristic(State initial_state) {
        // Here's a chance to pre-process the static parts of the level.
    }

    virtual int h(State state) {
        return 0;
    }

    virtual int f(State state) = 0;
    virtual std::string toString() = 0;
    virtual ~Heuristic() = default;
};

class HeuristicAStar : public Heuristic {
   public:
    HeuristicAStar(State initial_state) : Heuristic(initial_state) {
        // Here's a chance to pre-process the static parts of the level.
    }

    int f(State state) override;

    std::string toString() override {
        return "A* evaluation";
    }
};

class HeuristicWeightedAStar : public Heuristic {
   public:
    HeuristicWeightedAStar(State initial_state, int w) : Heuristic(initial_state), w(w) {
        // Here's a chance to pre-process the static parts of the level.
    }

    int f(State state) override {
        return state.getG() + w * h(state);
    }

    std::string toString() override {
        return "WA*(" + std::to_string(w) + ") evaluation";
    }

   private:
    int w;
};

class HeuristicGreedy : public Heuristic {
   public:
    HeuristicGreedy(State initial_state) : Heuristic(initial_state) {
        // Here's a chance to pre-process the static parts of the level.
    }

    int f(State state) override {
        return h(state);
    }

    std::string toString() override {
        return "greedy evaluation";
    }
};
