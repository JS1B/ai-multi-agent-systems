#include "SingleBoxPDBHeuristicCalculator.hpp"
#include "point2d.hpp"
// #include "utils.hpp" // REMOVED - File not found and logic simplified

#include <stdexcept> // For std::runtime_error
#include <iostream> // For debugging PDB generation
#include <queue> // For std::queue in PDB generation
#include <set>   // For visited set in PDB generation (or use map keys)
#include <algorithm> // For std::reverse (if needed)
#include <vector> // Ensure vector is included for precomputed_push_sequences_

namespace HeuristicCalculators {

SingleBoxPDBHeuristicCalculator::SingleBoxPDBHeuristicCalculator(const Level& level, char target_box_id, char target_goal_id)
    : HeuristicCalculator(level), // Call base constructor
      level_ref_(level),
      target_box_id_(target_box_id),
      target_goal_id_(target_goal_id) { // target_goal_id is now the specific goal char for this box's PDB
    
    bool found_goal_pos = false;
    // Iterate through the goals map using the character ID of the Goal objects.
    // The target_goal_id provided to this constructor is the specific goal character ID
    // that this PDB instance should target.
    for (const auto& goal_pair : level_ref_.getGoalsMap()) {
        if (goal_pair.second.id() == target_goal_id_) { 
            box_goal_pos_ = goal_pair.second.position();
            found_goal_pos = true;
            break;
        }
    }

    if (!found_goal_pos) {
        // This should ideally not happen if searchclient.cpp correctly finds a goal_char_for_box
        // and that char exists in the goals map.
        std::string error_msg = "Could not find goal position for PDB. Target Box ID: " + 
                                std::string(1, target_box_id_) + 
                                ", Provided Target Goal ID for PDB: " + std::string(1, target_goal_id_);
        throw std::runtime_error(error_msg);
    }

    generate_pdb();
}

std::string SingleBoxPDBHeuristicCalculator::getName() const {
    return "pdb_" + std::string(1, target_box_id_);
}

void SingleBoxPDBHeuristicCalculator::generate_pdb() {
    precomputed_push_sequences_.clear();
    std::queue<Point2D> q;

    q.push(box_goal_pos_);
    precomputed_push_sequences_[box_goal_pos_] = {};

    Point2D box_move_deltas[] = {Point2D(0, 1), Point2D(0, -1), Point2D(1, 0), Point2D(-1, 0)}; 
    ActionType push_action_type = ActionType::Push;

    while(!q.empty()){ 
        Point2D current_box_pos = q.front();
        q.pop(); 

        std::vector<ActionType> path_to_current = precomputed_push_sequences_[current_box_pos];

        for (int i = 0; i < 4; ++i) { 
            Point2D previous_box_pos = current_box_pos + box_move_deltas[i];

            if (!(previous_box_pos.y() >= 0 && previous_box_pos.y() < level_ref_.getRows() &&
                  previous_box_pos.x() >= 0 && previous_box_pos.x() < level_ref_.getCols() &&
                  !level_ref_.isWall(previous_box_pos.y(), previous_box_pos.x()))) {
                continue;
            }
            
            if (precomputed_push_sequences_.count(previous_box_pos)) {
                continue; 
            }

            std::vector<ActionType> new_path = path_to_current;
            new_path.insert(new_path.begin(), push_action_type); 
            
            precomputed_push_sequences_[previous_box_pos] = new_path;
            q.push(previous_box_pos);
        }
    }
    /* // Debug: Print PDB sizes or a few paths
    std::cout << "PDB for box " << target_box_id_ << " to goal " << target_goal_id_ << " generated. "
              << "Map size: " << precomputed_push_sequences_.size() << std::endl;
    for(const auto& pair : precomputed_push_sequences_){
        if(pair.second.size() < 2 && !pair.second.empty()){ // Print some short non-empty paths
            std::cout << "  Path from (" << pair.first.x << "," << pair.first.y << "): ";
            for(ActionType act : pair.second){
                std::cout << static_cast<int>(act) << " ";
            }
            std::cout << std::endl;
        }
    }
    */
}

int SingleBoxPDBHeuristicCalculator::calculateH(const State& state, const ActionCostMap& costs_for_this_heuristic) const {
    Point2D box_pos;
    bool box_found = false;
    for(const auto& box_pair : state.currentBoxes_){
        if(box_pair.first == target_box_id_){
            box_pos = box_pair.second.position(); // Corrected
            box_found = true;
            break;
        }
    }

    if (!box_found) {
        // Box isn't even on the map in the current state, shouldn't happen if level is consistent.
        return HeuristicCalculator::MAX_HEURISTIC_VALUE; 
    }

    if (box_pos == box_goal_pos_) {
        return 0; // Box is already at its goal
    }

    auto it = precomputed_push_sequences_.find(box_pos);
    if (it == precomputed_push_sequences_.end()) {
        // Box position not in PDB, means goal is unreachable from here according to PDB
        return HeuristicCalculator::MAX_HEURISTIC_VALUE;
    }

    const std::vector<ActionType>& push_sequence = it->second;
    if (push_sequence.empty() && box_pos != box_goal_pos_){
        // Should not happen if PDB is correct: empty path but not at goal
        return HeuristicCalculator::MAX_HEURISTIC_VALUE;
    }

    double h_double_value = 0.0;
    for (ActionType push_action : push_sequence) {
        auto cost_it = costs_for_this_heuristic.find(push_action);
        if (cost_it != costs_for_this_heuristic.end()) {
            h_double_value += cost_it->second;
        } else {
            // Action from PDB path has no remaining cost in the budget for this heuristic.
            // This means previous heuristics in SCP consumed it fully.
            // For PDB, this implies the step is "free" for this heuristic's calculation.
            // If the PDB was built on unit costs, one might add 1 here, but SCP handles partitioning.
            // So, if cost is not in the map, it's 0 for this heuristic's h-value contribution.
        }
    }
    if (h_double_value >= HeuristicCalculator::MAX_HEURISTIC_VALUE) {
        return HeuristicCalculator::MAX_HEURISTIC_VALUE;
    }
    return static_cast<int>(std::round(h_double_value));
}

ActionCostMap SingleBoxPDBHeuristicCalculator::saturate(const State& state, const ActionCostMap& available_costs) const {
    Point2D box_pos;
    bool box_found = false;
    for(const auto& box_pair : state.currentBoxes_){
        if(box_pair.first == target_box_id_){
            box_pos = box_pair.second.position();
            box_found = true;
            break;
        }
    }

    if (!box_found || box_pos == box_goal_pos_) {
        return {}; // Box not found or already at goal, consumes nothing
    }

    auto it = precomputed_push_sequences_.find(box_pos);
    if (it == precomputed_push_sequences_.end() || it->second.empty()) {
        // Goal unreachable according to PDB from this box_pos, or empty path (should only be for goal itself)
        return {}; // Consumes nothing
    }

    const std::vector<ActionType>& push_sequence = it->second;
    // Demand is the number of pushes in the sequence. Each PDB push action notionally costs 1 unit.
    double demanded_push_units = static_cast<double>(push_sequence.size());

    ActionCostMap consumed_costs;
    auto available_push_cost_it = available_costs.find(ActionType::Push);

    double actual_consumed_for_pushes = 0.0;
    if (available_push_cost_it != available_costs.end()) {
        double budget_for_pushes = available_push_cost_it->second;
        if (budget_for_pushes > 0) { // Only consume if there's a positive budget
            actual_consumed_for_pushes = std::min(demanded_push_units, budget_for_pushes);
        }
    }
    // else: No budget available for pushes, so consumes 0 for pushes.

    if (actual_consumed_for_pushes > 1e-9) { // Add to map only if meaningfully positive
        consumed_costs[ActionType::Push] = actual_consumed_for_pushes;
    }
    
    return consumed_costs;
}

} // namespace HeuristicCalculators 