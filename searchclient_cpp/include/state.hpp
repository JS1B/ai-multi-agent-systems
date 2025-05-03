#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <sstream>
#include <string>
#include <tuple>  // For lexicographical comparison @todo: Investigate
#include <vector>

#include "action.hpp"
#include "color.hpp"
#include "level.hpp"

class State {
   public:
    State() = delete;
    State(Level level);

    Level level;

    const State *parent;
    std::vector<const Action *> jointAction;

    inline int getG() const { return g_; }

    std::vector<std::vector<const Action *>> extractPlan() const;
    std::vector<State *> getExpandedStates() const;

    size_t getHash() const;
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;

    bool isGoalState() const;
    bool isApplicable(const Agent &agent, const Action &action) const;
    bool isConflicting(const std::vector<const Action *> &jointAction) const;

    std::string toString() const;

   private:
    // Creates a new state from a parent state and a joint action performed in that state.
    State(const State *parent, std::vector<const Action *> jointAction);

    const int g_;  // Cost of reaching this state
    mutable size_t hash_ = 0;

    // Returns true if the cell at the given position is free (i.e. not a wall, box, or agent)
    bool cellIsFree(const Point2D &position) const;

    // Returns the id of the agent at the given position, or 0
    char agentIdAt(const Point2D &position) const;

    // Returns the id of the box at the given position, or 0
    char boxIdAt(const Point2D &position) const;
};
