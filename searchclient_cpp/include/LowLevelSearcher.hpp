#pragma once

#include <vector>
#include <queue>    // For std::priority_queue
// #include <unordered_set> // Replaced by emhash9::HashSet
#include <memory>   // For std::shared_ptr
#include <functional> // For std::hash

#include "point2d.hpp"  // Corrected casing, moved before emhash
#include "Constraint.hpp" // For AgentPath, Constraint - Assuming this doesn't depend on emhash
#include "agent.hpp"      // Corrected casing, assuming file is agent.hpp - Assuming this doesn't depend on emhash
#include "level.hpp"      // Corrected casing, assuming file is level.hpp - Assuming this doesn't depend on emhash
#include "HeuristicCalculator.hpp" // For a basic heuristic like Manhattan Distance - Assuming this doesn't depend on emhash

#include "third_party/emhash/hash_table8.hpp" // For emhash8
#include "third_party/emhash/hash_set4.hpp"   // For emhash9
// #include "third_party/emhash/hash_set8.hpp" // Removed to avoid macro conflicts

struct LowLevelState {
    Point2D pos;                    // Current position of the agent
    int time;                       // Current time step
    int g;                          // Cost from start to this state (path length)
    int h;                          // Heuristic cost to goal
    std::shared_ptr<LowLevelState> parent; // To reconstruct the path

    LowLevelState(Point2D p, int t, int cost_g, int cost_h, std::shared_ptr<LowLevelState> pr = nullptr)
        : pos(p), time(t), g(cost_g), h(cost_h), parent(pr) {}

    int f() const { return g + h; }

    // For priority queue (min-heap)
    bool operator>(const LowLevelState& other) const {
        if (f() == other.f()) {
            if (h == other.h) { // Tie-break on g (prefer smaller g)
                 return g > other.g;
            }
            return h > other.h; // Tie-break on h (consistent tie-breaking)
        }
        return f() > other.f();
    }

    // For visited set (equality)
    bool operator==(const LowLevelState& other) const {
        return pos == other.pos && time == other.time;
    }
};

// Hash for LowLevelState for unordered_set/map
struct LowLevelStateHash {
    std::size_t operator()(const LowLevelState& s) const {
        std::size_t seed = 0;
        // Simple hash combining
        seed ^= std::hash<Point2D>{}(s.pos) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(s.time) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
     // Overload for shared_ptr for open list keying if needed, though raw state is better
    std::size_t operator()(const std::shared_ptr<LowLevelState>& s_ptr) const {
        if (!s_ptr) return 0; // Or some other default hash for nullptr
        return operator()(*s_ptr); // Call the hash for the raw state
    }
};

// Equality for shared_ptr<LowLevelState> for open list keying if needed
struct LowLevelStatePtrEqual {
    bool operator()(const std::shared_ptr<LowLevelState>& a, const std::shared_ptr<LowLevelState>& b) const {
        if (!a && !b) return true;
        if (!a || !b) return false;
        return *a == *b; // Call equality for the raw state
    }
};

// Comparator for priority queue storing shared_ptr<LowLevelState>
struct CompareLowLevelStatePtrs {
    bool operator()(const std::shared_ptr<LowLevelState>& a, const std::shared_ptr<LowLevelState>& b) const {
        if (!a || !b) return false; // Or handle as an error, or define order for nulls
        return *a > *b; // Uses LowLevelState::operator>
    }
};

class LowLevelSearcher {
public:
    LowLevelSearcher(const Level& level_data);

    // Finds a path for a single agent respecting constraints.
    // Returns the path and its cost (length).
    // If no path is found, path is empty and cost is -1 (or some indicator).
    std::pair<AgentPath, int> find_path(
        const Agent& agent_to_plan,
        const Point2D& goal_pos,
        const std::vector<Constraint>& constraints_for_agent,
        int max_time_limit = 256,
        int max_nodes_to_expand = 100000 // Safety break for very long searches
    );

private:
    const Level& level_data_;
    // Consider a simple heuristic calculator instance here if needed, e.g., Manhattan distance
    // HeuristicCalculators::ManhattanDistanceCalculator manhattan_calc_;

    int calculate_heuristic(const Point2D& current_pos, const Point2D& goal_pos) const;
    bool is_constrained(const Point2D& pos, int time, const std::vector<Constraint>& constraints) const;
    AgentPath reconstruct_path(const std::shared_ptr<LowLevelState>& goal_node) const;
}; 