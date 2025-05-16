#include "LowLevelSearcher.hpp"
#include <algorithm> // For std::reverse
#include <cmath> // For std::abs for Manhattan distance

LowLevelSearcher::LowLevelSearcher(const Level& level_data)
    : level_data_(level_data) {
    // Initialization, if any, for the searcher itself
    // For example, if you had a pre-allocated heuristic calculator object:
    // manhattan_calc_ = HeuristicCalculators::ManhattanDistanceCalculator(); 
    // (Assuming ManhattanDistanceCalculator has a default constructor or relevant one)
}

int LowLevelSearcher::calculate_heuristic(const Point2D& current_pos, const Point2D& goal_pos) const {
    // Simple Manhattan distance
    return std::abs(current_pos.x() - goal_pos.x()) + std::abs(current_pos.y() - goal_pos.y());
}

bool LowLevelSearcher::is_constrained(const Point2D& pos, int time, const std::vector<Constraint>& constraints) const {
    for (const auto& constraint : constraints) {
        // Assuming constraints_for_agent only contains constraints for the agent being planned
        // No need to check constraint.agent_id here if the vector is pre-filtered.
        if (constraint.location == pos && constraint.time_step == time) {
            return true;
        }
        // TODO: Add support for edge constraints if needed later
        // e.g., if constraint is (agent, loc1, loc2, time_step) meaning agent can't move from loc1 to loc2 at time_step
    }
    return false;
}

AgentPath LowLevelSearcher::reconstruct_path(const std::shared_ptr<LowLevelState>& goal_node) const {
    AgentPath path;
    std::shared_ptr<LowLevelState> current = goal_node;
    while (current != nullptr) {
        path.push_back({current->pos, current->time});
        current = current->parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::pair<AgentPath, int> LowLevelSearcher::find_path(
    const Agent& agent_to_plan,
    const Point2D& goal_pos,
    const std::vector<Constraint>& constraints_for_agent,
    int max_time_limit,
    int max_nodes_to_expand
) {
    // Priority queue for the open set (min-heap storing shared_ptrs)
    std::priority_queue<std::shared_ptr<LowLevelState>, 
                          std::vector<std::shared_ptr<LowLevelState>>, 
                          CompareLowLevelStatePtrs> open_set;

    // Stores g-cost for states. Key is LowLevelState (pos, time) for quick lookup.
    // We don't store the shared_ptr here directly as key because hash/equality might be tricky
    // or less efficient than on the value type. Value is the g-cost.
    emhash8::HashMap<LowLevelState, int, LowLevelStateHash> state_g_costs;

    // Visited set (closed set). Stores LowLevelState by value.
    std::unordered_set<LowLevelState, LowLevelStateHash> closed_set;

    Point2D start_pos = agent_to_plan.position();
    int start_h = calculate_heuristic(start_pos, goal_pos);
    auto current_state_ptr = std::make_shared<LowLevelState>(start_pos, 0, 0, start_h, nullptr);

    open_set.push(current_state_ptr);
    state_g_costs[*current_state_ptr] = 0;

    int nodes_expanded = 0;

    const std::vector<Point2D> actions = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0}, {0, 0} // Up, Down, Left, Right, Wait
    };

    while (!open_set.empty()) {
        current_state_ptr = open_set.top(); // Now current_state_ptr is a shared_ptr
        open_set.pop();

        // Check if already expanded (if we found a shorter path to it after it was closed, 
        // which shouldn't happen with a consistent heuristic, but good for robustness or non-strict graph search)
        // Or, more simply, if it's in closed set, we've processed it.
        if (closed_set.count(*current_state_ptr)) {
            continue;
        }
        closed_set.emplace(*current_state_ptr); // Add to closed set by value
        nodes_expanded++;

        // Goal check
        if (current_state_ptr->pos == goal_pos) {
            AgentPath path = reconstruct_path(current_state_ptr);
            return {path, current_state_ptr->g};
        }

        if (current_state_ptr->time >= max_time_limit || nodes_expanded >= max_nodes_to_expand) {
            continue;
        }

        // Generate successors
        for (const auto& action : actions) {
            Point2D next_pos = {
                static_cast<int16_t>(current_state_ptr->pos.x() + action.x()), 
                static_cast<int16_t>(current_state_ptr->pos.y() + action.y())
            };
            int next_time = current_state_ptr->time + 1;
            int next_g = current_state_ptr->g + 1;

            if (level_data_.isWall(next_pos.y(), next_pos.x())) {
                continue;
            }

            if (is_constrained(next_pos, next_time, constraints_for_agent)) {
                continue;
            }

            // Create the successor state value for lookups/insertions
            // Parent is current_state_ptr, which is the shared_ptr to the predecessor.
            auto successor_state_ptr = std::make_shared<LowLevelState>(next_pos, next_time, next_g, 
                                                                     calculate_heuristic(next_pos, goal_pos), 
                                                                     current_state_ptr); // Correct parentage

            // If in closed_set, skip
            if (closed_set.count(*successor_state_ptr)) {
                continue;
            }

            // Check if successor is in open_set (via state_g_costs) and if this path is better
            auto it = state_g_costs.find(*successor_state_ptr);
            if (it != state_g_costs.end() && it->second <= next_g) {
                continue; // Already in open set with a better or equal g-cost
            }
            
            // This path is better or new, add/update in open_set and state_g_costs
            state_g_costs[*successor_state_ptr] = next_g;
            open_set.push(successor_state_ptr);
        }
    }

    return {{}, -1}; // No path found
}

// The LowLevelState priority queue should store std::shared_ptr<LowLevelState> for correct parent tracking.
// And the comparison for the PQ needs to dereference the pointers.
// e.g. struct CompareLowLevelStatePtrs { bool operator()(const auto&a, const auto&b){ return *a > *b;}};
// std::priority_queue<std::shared_ptr<LowLevelState>, std::vector<std::shared_ptr<LowLevelState>>, CompareLowLevelStatePtrs> open_set_ptr;
// And open_set_costs would map LowLevelState -> std::shared_ptr<LowLevelState> or store g-cost with shared_ptr elsewhere.
// This implementation has a known issue with parent tracking if PQ stores values. 