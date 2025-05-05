#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <sstream>
#include <string>
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

// Custom hash function for State pointers
struct StatePtrHash {
    std::size_t operator()(const State *state_ptr) const {
        // Ensure the pointer is not null before dereferencing
        if (!state_ptr) {
            // Handle null pointer case, e.g., return a specific hash value
            // or throw an exception, depending on your logic.
            // Returning 0 is simple but might collide with the cached hash=0 state.
            return 0;
        }
        // Dereference the pointer and call the State's hashing method
        return state_ptr->getHash();
    }
};

// Custom equality operator for State pointers
struct StatePtrEqual {
    bool operator()(const State *lhs_ptr, const State *rhs_ptr) const {
        // Handle null pointers
        if (lhs_ptr == rhs_ptr) {  // Both null or same pointer
            return true;
        }
        if (!lhs_ptr || !rhs_ptr) {  // One is null, the other isn't
            return false;
        }
        // Dereference pointers and use the State's equality operator
        return *lhs_ptr == *rhs_ptr;
    }
};