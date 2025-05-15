#include "HeuristicCalculator.hpp"
#include <vector>
#include <queue>
#include <map> // For visited set in BFS, or use std::vector<std::vector<bool>>
#include <cmath> // For std::abs in Manhattan
#include <limits.h> // For INT_MAX
#include "box.hpp" // Added for Box type
#include <functional>

namespace HeuristicCalculators {

const int HeuristicCalculator::MAX_HEURISTIC_VALUE; // Definition for the static const member

// ZeroHeuristicCalculator
int ZeroHeuristicCalculator::calculateH(const State& state) {
    (void)state; // Unused
    return 0;
}

std::string ZeroHeuristicCalculator::getName() const {
    return "Zero";
}

// ManhattanDistanceCalculator
int ManhattanDistanceCalculator::calculateH(const State& state) {
    int total_manhattan_distance = 0;
    for (const auto& agent_pair : state.currentAgents_) {
        const Agent& agent = agent_pair.second;
        Point2D goal_pos = state.level_.getGoalPosition(agent.getId());
        if (goal_pos.x() != -1 || goal_pos.y() != -1) {
            total_manhattan_distance += std::abs(agent.position().y() - goal_pos.y()) +
                                        std::abs(agent.position().x() - goal_pos.x());
        }
    }
    return total_manhattan_distance;
}

std::string ManhattanDistanceCalculator::getName() const {
    return "ManhattanDistanceSum";
}

namespace { // Anonymous namespace for bfs_pathfinding helper
static std::pair<int, std::vector<const Action*>> bfs_pathfinding_helper(
    Point2D p_start_pos, 
    Point2D p_end_pos,
    const State& p_current_state, 
    const Level& p_level_param, 
    std::function<int(ActionType)> p_action_cost_fn,
    std::optional<char> p_agent_id_char_opt
) {
    if (p_start_pos == p_end_pos) return {0, {}};
    std::queue<std::pair<Point2D, std::pair<int, std::vector<const Action*>>>> q;
    q.push({p_start_pos, {0, {}}});
    std::map<Point2D, int> min_costs;
    min_costs[p_start_pos] = 0;
    std::vector<const Action*> move_actions;
    for(const auto* act_ptr : Action::allValues()){
        if(act_ptr->type == ActionType::Move) {
            move_actions.push_back(act_ptr);
        }
    }
    while (!q.empty()) {
        Point2D current_point = q.front().first;
        int current_path_cost = q.front().second.first;
        std::vector<const Action*> current_path_actions = q.front().second.second;
        q.pop();
        if (current_point == p_end_pos) {
            return {current_path_cost, current_path_actions};
        }
        for (const Action* move_action : move_actions) {
            Point2D next_point = current_point + move_action->agentDelta;
            if (next_point.y() >= 0 && next_point.y() < p_level_param.getRows() &&
                next_point.x() >= 0 && next_point.x() < p_level_param.getCols() &&
                !p_level_param.isWall(next_point.y(), next_point.x())) {
                bool cell_occupied_by_other_agent = false;
                if (p_agent_id_char_opt.has_value()) {
                    for(const auto& agent_pair : p_current_state.currentAgents_){
                        if (agent_pair.first != p_agent_id_char_opt.value() && agent_pair.second.position() == next_point){
                            cell_occupied_by_other_agent = true;
                            break;
                        }
                    }
                }
                bool cell_occupied_by_box = false;
                 for(const auto& box_pair : p_current_state.currentBoxes_){
                    if(box_pair.second.position() == next_point){
                        cell_occupied_by_box = true;
                        break;
                    }
                }
                if (cell_occupied_by_other_agent || cell_occupied_by_box) {
                    continue; 
                }
                int cost_of_this_action = p_action_cost_fn(move_action->type);
                int new_total_cost = current_path_cost + cost_of_this_action;
                if (!min_costs.count(next_point) || new_total_cost < min_costs[next_point]) {
                    min_costs[next_point] = new_total_cost;
                    std::vector<const Action*> new_path_actions = current_path_actions;
                    new_path_actions.push_back(move_action);
                    q.push({next_point, {new_total_cost, new_path_actions}});
                }
            }
        }
    }
    return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}};
}
} // end anonymous namespace

// Definition of HeuristicCalculator::bfs is now correctly inside HeuristicCalculators namespace
std::pair<int, std::vector<const Action*>> HeuristicCalculator::bfs(
    Point2D start_pos,         
    Point2D end_pos,
    const State& current_state,
    const Level& level_param,
    std::function<int(ActionType)> action_cost_fn,
    std::optional<char> agent_id_char_opt
) {
    return bfs_pathfinding_helper(start_pos, end_pos, current_state, level_param, action_cost_fn, agent_id_char_opt);
}

// SumOfIndividualCostsCalculator
int SumOfIndividualCostsCalculator::calculateH(const State& state) { 
    int total_h = 0;
    for (char agent_char : state.getAgentChars()) { 
        auto agent_it = state.currentAgents_.find(agent_char); 
        if (agent_it == state.currentAgents_.end()) continue; 
        const Agent& current_agent = agent_it->second;
        Point2D agent_pos = current_agent.position(); 

        Point2D goal_pos = this->currentLevel_.getGoalPosition(agent_char);
        if (goal_pos.x() == -1 && goal_pos.y() == -1) { 
            continue;
        }
        std::function<int(ActionType)> default_action_cost_fn = [](ActionType /*type*/) { return 1; }; 
        std::pair<int, std::vector<const Action*>> path_info = HeuristicCalculator::bfs(
            agent_pos, goal_pos, state, this->currentLevel_, default_action_cost_fn, agent_char
        );
        if (path_info.first == MAX_HEURISTIC_VALUE) {
            return MAX_HEURISTIC_VALUE; 
        }
        total_h += path_info.first;
    }
    return total_h;
}

int SumOfIndividualCostsCalculator::calculate_single_agent_path_cost(
    const Point2D& start_pos,
    const Point2D& goal_pos,
    int num_rows,
    int num_cols,
    const std::vector<std::vector<bool>>& walls_param) const { 
    if (start_pos == goal_pos) {
        return 0;
    }
    std::queue<std::pair<Point2D, int>> q;
    if (!(start_pos.y() >= 0 && start_pos.y() < num_rows && start_pos.x() >= 0 && start_pos.x() < num_cols)) {
        return INT_MAX; 
    }
    if (walls_param[start_pos.y()][start_pos.x()]) {
        return INT_MAX;
    }
    if (!(goal_pos.y() >= 0 && goal_pos.y() < num_rows && goal_pos.x() >= 0 && goal_pos.x() < num_cols)) {
        return INT_MAX;
    }
    q.push({start_pos, 0});
    std::vector<std::vector<bool>> visited(num_rows, std::vector<bool>(num_cols, false));
    visited[start_pos.y()][start_pos.x()] = true;
    int dr[] = {-1, 1, 0, 0}; 
    int dc[] = {0, 0, -1, 1}; 
    while (!q.empty()) {
        Point2D current_pos = q.front().first;
        int current_dist = q.front().second;
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Point2D next_pos(current_pos.x() + dc[i], current_pos.y() + dr[i]);
            if (next_pos.y() >= 0 && next_pos.y() < num_rows &&
                next_pos.x() >= 0 && next_pos.x() < num_cols &&
                !walls_param[next_pos.y()][next_pos.x()] && 
                !visited[next_pos.y()][next_pos.x()]) {
                if (next_pos == goal_pos) {
                    return current_dist + 1;
                }
                visited[next_pos.y()][next_pos.x()] = true;
                q.push({next_pos, current_dist + 1});
            }
        }
    }
    return INT_MAX; 
}

std::pair<int, std::vector<const Action*>> SumOfIndividualCostsCalculator::calculateSingleAgentPath(
    const State& state,
    char agent_id,
    const std::function<int(ActionType)>& action_cost_fn,
    const Level& level_ref
) {
    auto agent_it = state.currentAgents_.find(agent_id); 
    if (agent_it == state.currentAgents_.end()) {
        return {MAX_HEURISTIC_VALUE, {}}; 
    }
    const Agent& current_agent = agent_it->second;
    Point2D agent_pos = current_agent.position(); 

    Point2D goal_pos = level_ref.getGoalPosition(agent_id);
    if (goal_pos.x() == -1 && goal_pos.y() == -1) { 
        return {0, {}};
    }

    return HeuristicCalculator::bfs(
        agent_pos, goal_pos, state, level_ref, action_cost_fn, agent_id
    );
}

// BoxGoalManhattanDistanceCalculator
int BoxGoalManhattanDistanceCalculator::calculateH(const State& state) { 
    int total_h = 0;
    for (char box_char : state.getBoxChars()) { 
        int box_h = this->calculateSingleBoxManhattanDistance(state, box_char, this->currentLevel_);
        if (box_h == MAX_HEURISTIC_VALUE) {
            return MAX_HEURISTIC_VALUE; 
        }
        total_h += box_h;
    }
    return total_h;
}

int BoxGoalManhattanDistanceCalculator::calculateSingleBoxManhattanDistance(
    const State& state,
    char box_id,
    const Level& level_ref
) {
    auto box_it = state.currentBoxes_.find(box_id); 
    if (box_it == state.currentBoxes_.end()) {
        return MAX_HEURISTIC_VALUE; 
    }
    const Box& current_box = box_it->second;
    Point2D box_pos = current_box.position(); 

    Point2D goal_pos = level_ref.getGoalPosition(box_id);
    if (goal_pos.x() == -1 && goal_pos.y() == -1) {
        return 0; 
    }
    int dist = std::abs(box_pos.x() - goal_pos.x()) + std::abs(box_pos.y() - goal_pos.y()); 
    return dist;
}

// getName for BoxGoalManhattanDistanceCalculator is inlined in header, so removed from .cpp

} // namespace HeuristicCalculators 