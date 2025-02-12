#include "heuristic.h"

Heuristic::Heuristic(State initial_state)
{
    // Here's a chance to pre-process the static parts of the level.
}

int Heuristic::h(State state)
{
    return 0; // Default heuristic value
}

// HeuristicAStar
HeuristicAStar::HeuristicAStar(State initial_state) : Heuristic(initial_state) {}

int HeuristicAStar::f(State state)
{
    return state.getG() + h(state);
}

std::string HeuristicAStar::toString()
{
    return "A* evaluation";
}

// HeuristicWeightedAStar
HeuristicWeightedAStar::HeuristicWeightedAStar(State initial_state, int w) : Heuristic(initial_state), w(w) {}

int HeuristicWeightedAStar::f(State state)
{
    return state.getG() + w * h(state);
}

std::string HeuristicWeightedAStar::toString()
{
    return "WA*(" + std::to_string(w) + ") evaluation";
}

// HeuristicGreedy
HeuristicGreedy::HeuristicGreedy(State initial_state) : Heuristic(initial_state) {}

int HeuristicGreedy::f(State state)
{
    return h(state);
}

std::string HeuristicGreedy::toString()
{
    return "greedy evaluation";
}