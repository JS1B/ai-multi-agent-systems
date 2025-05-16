#pragma once

#include <vector>
#include <unordered_map>
#include <memory> // For std::shared_ptr or std::unique_ptr if nodes form a tree explicitly

#include "Constraint.hpp" // Defines Constraint, AgentPath, SolutionPaths
#include "agent.hpp"      // For Agent::IdType or agent identifiers

struct HighLevelNode {
    std::vector<Constraint> constraints; // All constraints active for this node
    SolutionPaths solution;              // Paths for all agents under these constraints
    int sum_of_costs;                    // Sum of individual agent path costs (g-value of HL node)
    // int heuristic_cost;               // Heuristic for this node (e.g., number of conflicts, for ICBS)

    // Optional: Parent pointer if you want to reconstruct the CT later, not strictly needed for basic CBS
    // std::weak_ptr<HighLevelNode> parent; 

    HighLevelNode() : sum_of_costs(0) /*, heuristic_cost(0)*/ {}

    // Copy constructor might be useful if nodes are copied, 
    // or make it explicit if using shared_ptr for solution paths to avoid deep copies.
    // For now, default copy/move should be okay if SolutionPaths handles itself well.

    // Comparison operator for priority queue (we want to extract min sum_of_costs)
    // If also using heuristic for tie-breaking or for ICBS, expand this.
    bool operator>(const HighLevelNode& other) const {
        if (sum_of_costs == other.sum_of_costs) {
            // Tie-breaking: prefer fewer constraints, or could be based on heuristic_cost
            // return constraints.size() > other.constraints.size(); 
            return false; // Simple tie-breaking for now
        }
        return sum_of_costs > other.sum_of_costs;
    }

    // Potentially add a method to calculate ID/hash for visited list in high-level search
    // based on the set of constraints (which needs to be canonical).
}; 