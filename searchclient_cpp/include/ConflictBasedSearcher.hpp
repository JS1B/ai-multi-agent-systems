#pragma once

#include <vector>
#include <queue>    // For std::priority_queue
#include <memory>   // For std::shared_ptr
#include <map> // For ordered map of agents if needed

#include "third_party/emhash/hash_table8.hpp" // Added for emhash

// #include "Level.hpp"
#include "level.hpp" // Corrected casing
#include "agent.hpp"
#include "Constraint.hpp"      // For SolutionPaths, AgentPath, PathEntry
#include "HighLevelNode.hpp"   // Defines HighLevelNode
#include "LowLevelSearcher.hpp"// For the low-level A* search
#include "CBSConflict.hpp"     // For CBSConflict definition

// Comparator for HighLevelNode shared_ptrs for the priority queue
// Orders by sum_of_costs, then potentially by number of constraints (fewer is better) as a tie-breaker.
struct CompareHighLevelNodes {
    bool operator()(const std::shared_ptr<HighLevelNode>& a, const std::shared_ptr<HighLevelNode>& b) const {
        if (!a || !b) return false; // Or handle error
        if (a->sum_of_costs == b->sum_of_costs) {
            // Tie-break: prefer nodes with fewer constraints (simpler, closer to root)
            // Could also use a high-level heuristic here if implementing ICBS
            return a->constraints.size() > b->constraints.size();
        }
        return a->sum_of_costs > b->sum_of_costs; // Min-heap based on sum_of_costs
    }
};

class ConflictBasedSearcher {
public:
    ConflictBasedSearcher(const Level& initial_level_state);

    // Attempts to find a conflict-free set of paths for all agents.
    // Returns the solution (paths for all agents) and the total sum of costs.
    // If no solution found (e.g., within limits), solution paths might be empty and cost -1.
    std::pair<SolutionPaths, int> find_solution(
        int max_hl_nodes_to_expand = 10000, // Safety break for high-level search
        int low_level_time_limit = 256,     // Max time steps for low-level paths
        int low_level_node_limit = 100000   // Max nodes for low-level search expansion
    );

private:
    const Level& level_data_;
    std::vector<Agent> agents_; // Copy of agents or references, with their goals
    emhash8::HashMap<char, Point2D> agent_goals_; // Quick lookup for agent goals using emhash
    LowLevelSearcher low_level_searcher_;

    // Calculates the sum of costs (path lengths) for a given solution.
    int calculate_sum_of_costs(const SolutionPaths& solution) const;

    // Detects all conflicts in a given set of paths.
    // Returns true if conflicts exist, false otherwise. Populates the conflicts_list.
    bool find_conflicts(const SolutionPaths& current_solution, std::vector<CBSConflict>& conflicts_list) const;

    // Helper to get all constraints applicable to a specific agent from a list of constraints.
    std::vector<Constraint> get_constraints_for_agent(char agent_id, const std::vector<Constraint>& all_constraints) const;
    
    // Helper to find the position of an agent at a given time step from its path.
    // Returns std::nullopt if agent is not at that time or path is too short.
    std::optional<Point2D> get_location_at_time(const AgentPath& path, int time) const;
}; 