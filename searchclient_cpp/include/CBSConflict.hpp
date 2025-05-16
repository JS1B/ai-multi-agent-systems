#pragma once

#include <vector>
#include <optional> // For optional second location in edge conflicts

#include "point2d.hpp"
#include "Constraint.hpp" // To generate constraints from conflicts
#include "agent.hpp"      // For Agent::IdType or agent identifiers

enum class ConflictType {
    VERTEX, // Two agents at the same location at the same time
    EDGE    // Two agents traversing the same edge in opposite directions at the same time (swapping)
    // TODO: Potentially add FOLLOWING_TOO_CLOSE if needed
};

struct CBSConflict {
    char agent1_id;         // ID of the first agent in conflict
    char agent2_id;         // ID of the second agent in conflict
    Point2D location1;      // Location of the conflict (for vertex, or first location for edge)
    std::optional<Point2D> location2; // Second location for edge conflicts (e.g. agent1 at loc1, agent2 at loc2, they swap)
    int time_step;          // Time step at which the conflict occurs
    ConflictType type;      // Type of the conflict

    CBSConflict(char id1, char id2, Point2D loc1, int time, ConflictType conflict_type = ConflictType::VERTEX)
        : agent1_id(id1), agent2_id(id2), location1(loc1), location2(std::nullopt), time_step(time), type(conflict_type) {}

    CBSConflict(char id1, char id2, Point2D loc1, Point2D loc2, int time)
        : agent1_id(id1), agent2_id(id2), location1(loc1), location2(loc2), time_step(time), type(ConflictType::EDGE) {}

    // Generates two constraints from this conflict, one for each agent.
    // For a vertex conflict at (loc, t) for agents A1, A2:
    //   - Constraint 1: A1 cannot be at loc at time t
    //   - Constraint 2: A2 cannot be at loc at time t
    // For an edge conflict where A1 moves loc1->loc2 and A2 moves loc2->loc1 at time t (meaning A1 at loc2 at t+1, A2 at loc1 at t+1):
    //   - Constraint 1: A1 cannot be at loc2 at time t+1 (if coming from loc1)
    //   - Constraint 2: A2 cannot be at loc1 at time t+1 (if coming from loc2)
    // Note: Edge constraints are more complex; for now, focusing on vertex constraints derived from vertex conflicts.
    // The constraints generated here will be simple vertex constraints.
    std::pair<Constraint, Constraint> get_constraints() const {
        if (type == ConflictType::VERTEX) {
            // Agent1 cannot be at location1 at time_step
            Constraint c1(agent1_id, location1, time_step);
            // Agent2 cannot be at location1 at time_step
            Constraint c2(agent2_id, location1, time_step);
            return {c1, c2};
        } else if (type == ConflictType::EDGE && location2.has_value()) {
            // For an edge conflict where agent1 was at location1 (at time_step-1) and wants to move to location2 (at time_step)
            // and agent2 was at location2 (at time_step-1) and wants to move to location1 (at time_step)
            // Constraint for agent1: cannot be at location2 at time_step
            Constraint c1(agent1_id, location2.value(), time_step);
            // Constraint for agent2: cannot be at location1 at time_step
            Constraint c2(agent2_id, location1, time_step);
            return {c1, c2};
        }
        // Fallback or error - should not happen if conflict is well-defined
        // Return two dummy/invalid constraints
        // This indicates an issue if reached, as a valid conflict should produce valid constraints.
        // For robustness, one might throw an exception or return std::optional<std::pair<...>>
        fprintf(stderr, "ERROR: Trying to get constraints from an ill-defined conflict.\n");
        return {Constraint('?', Point2D(-1,-1), -1), Constraint('?', Point2D(-1,-1), -1)};
    }

    // Equality operator for comparing conflicts, e.g. for storing in a set of unique conflicts
    bool operator==(const CBSConflict& other) const {
        return agent1_id == other.agent1_id &&
               agent2_id == other.agent2_id &&
               location1 == other.location1 &&
               location2 == other.location2 && // handles std::optional comparison correctly
               time_step == other.time_step &&
               type == other.type;
    }
};

// Hash for CBSConflict if we need to store them in unordered_sets/maps
// (Not strictly necessary for basic CBS high-level node, but good for more advanced techniques)
struct CBSConflictHash {
    std::size_t operator()(const CBSConflict& conflict) const {
        std::size_t seed = 0;
        seed ^= std::hash<char>()(conflict.agent1_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<char>()(conflict.agent2_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<Point2D>{}(conflict.location1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        if (conflict.location2.has_value()) {
            seed ^= std::hash<Point2D>{}(conflict.location2.value()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        seed ^= std::hash<int>()(conflict.time_step) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(static_cast<int>(conflict.type)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
}; 