#pragma once

#include <vector>
#include <string> // For agent ID, assuming char or std::string
#include <utility> // For std::pair

#include "point2d.hpp" // Assuming Point2D is used for locations

// Represents a constraint on an agent's movement.
// For now, a simple vertex constraint: agent_id cannot be at 'location' at 'time_step'.
struct Constraint {
    char agent_id;    // The ID of the agent this constraint applies to
    Point2D location; // The constrained location
    int time_step;    // The time step at which the location is constrained

    Constraint(char aid, Point2D loc, int t) 
        : agent_id(aid), location(loc), time_step(t) {}

    // Equality operator for comparing constraints, useful for storing in sets or lists
    bool operator==(const Constraint& other) const {
        return agent_id == other.agent_id &&
               location == other.location &&
               time_step == other.time_step;
    }

    // For hashing constraints if needed (e.g., if stored in unordered_set)
    struct Hash {
        std::size_t operator()(const Constraint& c) const {
            std::size_t seed = 0;
            // A simple way to combine hashes. 
            // Consider a more robust hash combining function if many hash collisions occur.
            seed ^= std::hash<char>()(c.agent_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<Point2D>{}(c.location) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>()(c.time_step) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
};

// Represents a single step in an agent's path (location at a specific time)
using PathEntry = std::pair<Point2D, int>; // {location, time_step}

// Represents a path for a single agent
using AgentPath = std::vector<PathEntry>;

// For storing paths of all agents
// #include <unordered_map> // Or std::map if agent IDs are ordered meaningfully beyond char
#include "third_party/emhash/hash_table8.hpp" // Using emhash8 now
#include "agent.hpp" // Corrected casing: For Agent::IdType if you have one, otherwise char is fine

// Using char for agent_id directly as in Constraint
// using SolutionPaths = std::unordered_map<char, AgentPath>;
using SolutionPaths = emhash8::HashMap<char, AgentPath>; 