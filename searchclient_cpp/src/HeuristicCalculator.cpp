#include "HeuristicCalculator.hpp"
#include <vector>
#include <queue>
#include <map> // For visited set in BFS, or use std::vector<std::vector<bool>>
#include <cmath> // For std::abs in Manhattan
#include <limits.h> // For INT_MAX
#include "box.hpp" // Added for Box type
#include <functional>

namespace HeuristicCalculators {

// Helper function to subtract action costs
ActionCostMap subtractCosts(const ActionCostMap& main_costs, const ActionCostMap& costs_to_subtract) {
    ActionCostMap result = main_costs;
    for (const auto& pair_to_subtract : costs_to_subtract) {
        ActionType type = pair_to_subtract.first;
        double amount_to_subtract = pair_to_subtract.second;

        auto it = result.find(type);
        if (it != result.end()) {
            // Ensure cost does not go below zero.
            it->second = std::max(0.0, it->second - amount_to_subtract);
        } else {
            // If the type to subtract is not in main_costs, it means we are subtracting from an implicit 0.
            // The result for this type should remain 0 (or absent), not become negative.
            // No action needed here if we only store positive costs, or ensure it's not added if amount_to_subtract > 0
        }
    }
    // Remove entries with zero or very small cost to keep the map clean
    for (auto it_clean = result.begin(); it_clean != result.end(); /* no increment */) {
        if (it_clean->second <= 1e-9) { // Tolerance for floating point comparisons
            it_clean = result.erase(it_clean);
        } else {
            ++it_clean;
        }
    }
    return result;
}

const int HeuristicCalculator::MAX_HEURISTIC_VALUE; // Definition for the static const member

// ZeroHeuristicCalculator
// Removed ZeroHeuristicCalculator::calculateH and ZeroHeuristicCalculator::getName definitions
// as they are provided inline in HeuristicCalculator.hpp

// ManhattanDistanceCalculator
int ManhattanDistanceCalculator::calculateH(const State& state, const ActionCostMap& costs) const {
    (void)costs; // Costs not used yet in this basic MDS
    int total_manhattan_distance = 0;

    // Calculate sum of Manhattan distances for each agent to its closest goal of its ID type
    for (const auto& agent_pair : state.currentAgents_) {
        const Agent& agent = agent_pair.second;
        Point2D agent_pos = agent.position();
        int min_dist_for_agent = HeuristicCalculator::MAX_HEURISTIC_VALUE;

        // Check goals for this agent's ID
        // Assuming goalsMap_ in level_ stores Point2D directly for goals related to agent/box IDs
        auto goal_it = this->level_.goalsMap_for_Point2D_.find(agent.getId());
        if (goal_it != this->level_.goalsMap_for_Point2D_.end()) {
            const Point2D& goal_pos = goal_it->second;
            min_dist_for_agent = Point2D::manhattanDistance(agent_pos, goal_pos);
        } else {
            // If no specific goal for this agent ID, what should it be? 
            // For now, if no goal, its contribution is 0 or it implies an issue.
            // Or, if agents MUST reach a goal, this state might be unsolvable from its perspective.
            // Let's assume if an agent has no goal, its part of h is 0.
            min_dist_for_agent = 0;
        }
        
        if (min_dist_for_agent == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            // No goal found for this agent, or goal is unreachable (not applicable for basic MDS)
            // This case might indicate an issue or an unsolvable state from this agent's perspective
            // For now, let's return a max value if any agent can't find its goal path (if we had pathing)
            // With simple MDS, it just means no goal of that char ID was in goalsMap_for_Point2D_
            // If an agent is on the map but has no goal, its contribution is 0.
        } else {
            total_manhattan_distance += min_dist_for_agent;
        }
    }
    return total_manhattan_distance;
}

// TODO: Implement ManhattanDistanceCalculator::saturate
ActionCostMap ManhattanDistanceCalculator::saturate(const State& state, const ActionCostMap& available_costs) const {
    (void)state; // Unused for now
    // For basic MDS that doesn't consider action costs for distance, it consumes no action costs.
    // Or, if we assume MDS implies unit cost moves, it would consume costs along the manhattan path.
    // For SCP, we need to be more precise. If MDS is calculated just by grid distance,
    // it doesn't directly map to consuming specific *operator* costs unless we define that mapping.
    // Simplest for now: consumes no specific costs from available_costs, similar to ZeroHeuristic.
    ActionCostMap consumed_costs;
    for(const auto& cost_pair : available_costs){
        consumed_costs[cost_pair.first] = 0.0; // Consumes nothing
    }
    return consumed_costs;
}

std::string ManhattanDistanceCalculator::getName() const {
    return "mds";
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

    int num_rows = p_level_param.getRows();
    int num_cols = p_level_param.getCols();

    std::vector<std::vector<int>> min_costs_grid(num_rows, std::vector<int>(num_cols, HeuristicCalculator::MAX_HEURISTIC_VALUE));
    // visited_in_queue helps prevent redundant queue entries if a shorter path to a node already in queue is found.
    // More critical in Dijkstra/A* with re-opening nodes. For basic BFS with unit/positive costs,
    // the first time we extract from queue is optimal for that node. So, simple cost check is enough.
    // std::vector<std::vector<bool>> visited_in_queue(num_rows, std::vector<bool>(num_cols, false)); 

    std::queue<std::pair<Point2D, std::pair<int, std::vector<const Action*>>>> q;

    // Check start_pos validity
    if (p_start_pos.y() < 0 || p_start_pos.y() >= num_rows || p_start_pos.x() < 0 || p_start_pos.x() >= num_cols || p_level_param.isWall(p_start_pos.y(), p_start_pos.x())) {
        return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}}; // Invalid start
    }

    q.push({p_start_pos, {0, {}}});
    min_costs_grid[p_start_pos.y()][p_start_pos.x()] = 0;
    // visited_in_queue[p_start_pos.y()][p_start_pos.x()] = true;

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

        // If we extract a path costlier than already found for current_point (can happen if re-added),
        // or if we are at the goal with a path costlier than one already found to the goal (less likely here)
        if (current_path_cost > min_costs_grid[current_point.y()][current_point.x()]) {
            continue;
        }

        if (current_point == p_end_pos) {
            // First time reaching end_pos via BFS with positive costs is optimal path to it.
            return {current_path_cost, current_path_actions};
        }

        for (const Action* move_action : move_actions) {
            Point2D next_point = current_point + move_action->agentDelta;

            if (next_point.y() >= 0 && next_point.y() < num_rows &&
                next_point.x() >= 0 && next_point.x() < num_cols &&
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
                if (cost_of_this_action == HeuristicCalculator::MAX_HEURISTIC_VALUE) continue; // Unpassable action

                int new_total_cost = current_path_cost + cost_of_this_action;

                if (new_total_cost < min_costs_grid[next_point.y()][next_point.x()]) {
                    min_costs_grid[next_point.y()][next_point.x()] = new_total_cost;
                    std::vector<const Action*> new_path_actions = current_path_actions;
                    new_path_actions.push_back(move_action);
                    q.push({next_point, {new_total_cost, new_path_actions}});
                    // visited_in_queue[next_point.y()][next_point.x()] = true; 
                }
            }
        }
    }
    return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}}; // Target not reached
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
int SumOfIndividualCostsCalculator::calculateH(const State& state, const ActionCostMap& costs) const {
    int total_h_value = 0;
    // Define an action_cost_fn based on the passed 'costs' map
    auto action_cost_fn = [&](ActionType type) -> int {
        auto it = costs.find(type);
        if (it != costs.end()) {
            // Ensure cost is non-negative and fits in int. MAX_HEURISTIC_VALUE can signal unreachable.
            double cost_val = it->second;
            if (cost_val >= HeuristicCalculator::MAX_HEURISTIC_VALUE) return HeuristicCalculator::MAX_HEURISTIC_VALUE;
            if (cost_val < 0) return 1; // Or handle negative costs as per planning system rules, default to 1
            return static_cast<int>(cost_val);
        }
        return 1; // Default cost if action type not in the provided costs map
    };

    for (const auto& agent_pair : state.currentAgents_) {
        char agent_id = agent_pair.first;
        std::pair<int, std::vector<const Action*>> path_info =
            this->calculateSingleAgentPath(state, agent_id, action_cost_fn, this->level_);
        
        if (path_info.first == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            return HeuristicCalculator::MAX_HEURISTIC_VALUE; // Unsolvable for one agent, so total is max.
        }
        total_h_value += path_info.first;
        if (total_h_value >= HeuristicCalculator::MAX_HEURISTIC_VALUE) { // Prevent overflow
            return HeuristicCalculator::MAX_HEURISTIC_VALUE;
        }
    }
    return total_h_value;
}

// Implementation for SumOfIndividualCostsCalculator::calculate_and_saturate_for_agent
std::pair<int, ActionCostMap> SumOfIndividualCostsCalculator::calculate_and_saturate_for_agent(
    char agent_id,
    const State& state,
    const ActionCostMap& current_budget) const {
    
    Point2D start_pos;
    bool agent_found = false;
    for(const auto& agent_pair : state.currentAgents_){
        if(agent_pair.first == agent_id){
            start_pos = agent_pair.second.position();
            agent_found = true;
            break;
        }
    }
    if (!agent_found) return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}};

    Point2D goal_pos;
    auto specific_goal_it = this->level_.goalsMap_for_Point2D_.find(agent_id);
    if (specific_goal_it != this->level_.goalsMap_for_Point2D_.end()) {
        goal_pos = specific_goal_it->second;
    } else {
        return {0, {}}; // Agent has no goal, cost is 0, consumes nothing.
    }

    // Define action_cost_fn based on the current_budget
    auto action_cost_fn = [&](ActionType type) -> int {
        auto it = current_budget.find(type);
        if (it != current_budget.end()) {
            double cost_val = it->second;
            if (cost_val >= HeuristicCalculator::MAX_HEURISTIC_VALUE) return HeuristicCalculator::MAX_HEURISTIC_VALUE;
            // For saturation, if a cost is 0 in the budget, the action is free. If >0, it has that cost.
            // Negative costs in budget are not expected for SCP's remaining_costs.
            if (cost_val < 0) return HeuristicCalculator::MAX_HEURISTIC_VALUE; // Or treat as 1, or error. Let's say MAX if negative budget.
            return static_cast<int>(std::round(cost_val));
        }
        // If action type not in budget, it means its cost is effectively infinite for this path calculation.
        return HeuristicCalculator::MAX_HEURISTIC_VALUE; 
    };

    std::pair<int, std::vector<const Action*>> path_info = HeuristicCalculator::bfs(
        start_pos, goal_pos, state, this->level_, action_cost_fn, agent_id
    );

    if (path_info.first == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
        return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}};
    }

    ActionCostMap actual_consumed_from_budget;
    for (const Action* action : path_info.second) {
        auto cost_iter = current_budget.find(action->type);
        if (cost_iter != current_budget.end() && cost_iter->second < HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            actual_consumed_from_budget[action->type] += cost_iter->second; 
        } else {
            // This should not happen if BFS found a finite path using action_cost_fn derived from current_budget
            // Potentially an error or inconsistency
        }
    }
    // Sanity check, path_info.first should ideally match reconstructed_path_cost if using same rounding.
    return {path_info.first, actual_consumed_from_budget};
}

ActionCostMap SumOfIndividualCostsCalculator::saturate(const State& state, const ActionCostMap& available_costs) const {
    ActionCostMap total_consumed_costs_for_sic; // c_i for SIC
    ActionCostMap remaining_budget_for_agents = available_costs; // Track budget for subsequent agents if we do sequential saturation *within* SIC
                                                               // For now, assume each agent tries to saturate against the original available_costs for SIC.

    for (const auto& agent_pair : state.currentAgents_) {
        char agent_id = agent_pair.first;

        // For each agent, calculate its optimal path and consumed costs based on the *original* available_costs for SIC.
        // This is because SIC is one heuristic (h_i) in the SCP sequence. Its saturate method determines
        // the *total* c_i it claims. How it internally computes this (e.g., summing agent costs)
        // should be based on the full budget given to SIC's saturate method.
        std::pair<int, ActionCostMap> agent_saturation_info = 
            this->calculate_and_saturate_for_agent(agent_id, state, available_costs); // Pass full available_costs

        if (agent_saturation_info.first == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            // If one agent cannot reach its goal, SIC heuristic might be MAX_HEURISTIC_VALUE.
            // For saturation, this means this agent effectively consumes nothing *towards a solution path*,
            // or, it might imply the problem is unsolvable and it consumes all remaining budget in a sense.
            // Let's assume it consumes nothing if it can't find a path.
            continue; // Or handle as an indication that SIC itself is MAX_H and consumes all applicable costs.
        }

        // Add this agent's consumed costs to SIC's total consumed costs.
        for (const auto& cost_entry : agent_saturation_info.second) {
            total_consumed_costs_for_sic[cost_entry.first] += cost_entry.second;
        }
    }

    // Crucially, the returned total_consumed_costs_for_sic must not exceed available_costs for any action type.
    // The sum from agents might, if multiple agents use the same limited-cost action type.
    // The saturate contract is c_i(l) <= budget(l).
    // So, we must cap the summed total_consumed_costs_for_sic.
    ActionCostMap final_capped_consumed_costs;
    for(const auto& total_consumed_entry : total_consumed_costs_for_sic){
        ActionType type = total_consumed_entry.first;
        double demanded_by_sic = total_consumed_entry.second;

        auto budget_it = available_costs.find(type);
        if(budget_it != available_costs.end()){
            final_capped_consumed_costs[type] = std::min(demanded_by_sic, budget_it->second);
        } else {
            // Demanding cost for an action type not in budget means it consumes 0 of it.
            final_capped_consumed_costs[type] = 0.0;
        }
        // Clean up if effectively zero
        if (final_capped_consumed_costs[type] <= 1e-9) {
            final_capped_consumed_costs.erase(type);
        }
    }
    return final_capped_consumed_costs;
}

std::pair<int, std::vector<const Action*>> SumOfIndividualCostsCalculator::calculateSingleAgentPath(
    const State& state,
    char agent_id,
    const std::function<int(ActionType)>& action_cost_fn,
    const Level& level_ref // Pass level by const ref for safety
) const {
    Point2D start_pos;
    bool agent_found = false;
    for(const auto& agent_pair : state.currentAgents_){
        if(agent_pair.first == agent_id){
            start_pos = agent_pair.second.position();
            agent_found = true;
            break;
        }
    }
    if (!agent_found) return {HeuristicCalculator::MAX_HEURISTIC_VALUE, {}};

    Point2D goal_pos;
    // bool goal_found = false; // Removed: Unused variable
    // Find the goal for this specific agent
    auto specific_goal_it = level_ref.goalsMap_for_Point2D_.find(agent_id);
    if (specific_goal_it != level_ref.goalsMap_for_Point2D_.end()) {
        goal_pos = specific_goal_it->second;
        // goal_found = true; // Removed: Unused variable
    } else {
        // No specific goal for this agent, so its path cost is 0.
        return {0, {}};
    }
    // Call the static BFS helper
    return HeuristicCalculator::bfs(start_pos, goal_pos, state, level_ref, action_cost_fn, agent_id);
}

// BoxGoalManhattanDistanceCalculator
int BoxGoalManhattanDistanceCalculator::calculateH(const State& state, const ActionCostMap& costs) const {
    (void)costs; // Costs not directly used in basic BGM
    int total_h = 0;
    for (const auto& box_entry : state.currentBoxes_) {
        char box_id = box_entry.first;
        Point2D box_pos = box_entry.second.position();
        int min_dist_for_box = HeuristicCalculator::MAX_HEURISTIC_VALUE;

        // Find the goal for this box (assuming box_id maps to a goal type)
        auto goal_it = this->level_.goalsMap_for_Point2D_.find(box_id);
        if (goal_it != this->level_.goalsMap_for_Point2D_.end()) {
            const Point2D& goal_pos = goal_it->second;
            min_dist_for_box = Point2D::manhattanDistance(box_pos, goal_pos);
        } else {
            // Box has no designated goal of its own type, its contribution is 0.
            min_dist_for_box = 0; 
        }

        if (min_dist_for_box == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            // Should not happen with plain Manhattan unless box has no goal and we didn't set to 0.
            // For safety, if any box is "infinitely" far (e.g. no goal found and not handled),
            // this could indicate an unsolvable state for this heuristic component.
            // However, our logic above sets it to 0 if no goal.
        }
        total_h += min_dist_for_box;
    }
    return total_h;
}

ActionCostMap BoxGoalManhattanDistanceCalculator::saturate(const State& state, const ActionCostMap& available_costs) const {
    (void)state; // Unused for now, as BGM is purely geometric
    // Like MDS, if BGM is purely geometric, it consumes no specific action costs.
    ActionCostMap consumed_costs;
    for(const auto& cost_pair : available_costs){
        consumed_costs[cost_pair.first] = 0.0; // Consumes nothing
    }
    // Or, simply return an empty map: return {};
    // To be explicit about not consuming any of the *available* types:
    return consumed_costs; 
}

// Method for BGM that was in the header, ensure it's defined or removed if not used.
// It seems this was more of a helper concept for a different SCP design.
// For now, let's assume it's not directly used by the current SCP flow.
int BoxGoalManhattanDistanceCalculator::calculateSingleBoxManhattanDistance(
    const State& state,
    char box_id,
    const Level& level_ref 
) const {
    Point2D box_pos;
    bool box_found = false;
    for(const auto& box_pair : state.currentBoxes_){
        if(box_pair.first == box_id){
            box_pos = box_pair.second.position();
            box_found = true;
            break;
        }
    }
    if (!box_found) return HeuristicCalculator::MAX_HEURISTIC_VALUE; // Box not on map

    Point2D goal_pos;
    // bool goal_found = false; // Not needed here due to direct return logic
    auto specific_goal_it = level_ref.goalsMap_for_Point2D_.find(box_id);
    if (specific_goal_it != level_ref.goalsMap_for_Point2D_.end()) {
        goal_pos = specific_goal_it->second;
        // goal_found = true;
    } else {
        return 0; // Box has no specific goal, its MDS is 0.
    }
    return Point2D::manhattanDistance(box_pos, goal_pos);
}

} // namespace HeuristicCalculators 