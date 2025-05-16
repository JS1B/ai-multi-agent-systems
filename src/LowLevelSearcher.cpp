#include "LowLevelSearcher.hpp"
#include <algorithm> // For std::reverse
#include <cmath> // For std::abs for Manhattan distance

LowLevelSearcher::LowLevelSearcher(const Level& level_data)
    : level_data_(level_data) {
    // Initialization, if any, for the searcher itself
}

int LowLevelSearcher::calculate_heuristic(const Point2D& current_pos, const Point2D& goal_pos) const {
    return std::abs(current_pos.x() - goal_pos.x()) + std::abs(current_pos.y() - goal_pos.y());
}

bool LowLevelSearcher::is_constrained(const Point2D& pos, int time, const std::vector<Constraint>& constraints) const {
    for (const auto& constraint : constraints) {
        if (constraint.location == pos && constraint.time_step == time) {
            return true;
        }
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
    std::priority_queue<std::shared_ptr<LowLevelState>, 
                          std::vector<std::shared_ptr<LowLevelState>>, 
                          CompareLowLevelStatePtrs> open_set;

    emhash8::HashMap<LowLevelState, int, LowLevelStateHash> state_g_costs;

    // Visited set (closed set), implemented using HashMap. Stores LowLevelState by value.
    emhash8::HashMap<LowLevelState, bool, LowLevelStateHash> closed_set;

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
        current_state_ptr = open_set.top();
        open_set.pop();

        if (closed_set.count(*current_state_ptr)) {
            continue;
        }
        closed_set.emplace(*current_state_ptr, true); // Add to closed set
        nodes_expanded++;

        if (current_state_ptr->pos == goal_pos) {
            AgentPath path = reconstruct_path(current_state_ptr);
            return {path, current_state_ptr->g};
        }

        if (current_state_ptr->time >= max_time_limit || nodes_expanded >= max_nodes_to_expand) {
            continue;
        }

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

            auto successor_state_ptr = std::make_shared<LowLevelState>(next_pos, next_time, next_g, 
                                                                     calculate_heuristic(next_pos, goal_pos), 
                                                                     current_state_ptr);

            if (closed_set.count(*successor_state_ptr)) {
                continue;
            }

            auto it = state_g_costs.find(*successor_state_ptr);
            if (it != state_g_costs.end() && it->second <= next_g) {
                continue;
            }
            
            state_g_costs[*successor_state_ptr] = next_g;
            open_set.push(successor_state_ptr);
        }
    }

    return {{}, -1}; // No path found 