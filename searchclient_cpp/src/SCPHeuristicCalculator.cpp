#include "SCPHeuristicCalculator.hpp"
// #include "common.hpp" // Removed non-existent common.hpp
#include <algorithm> // For std::max

// Note: HeuristicCalculator.hpp (included by SCPHeuristicCalculator.hpp) brings in the HeuristicCalculators namespace
// so we don't need 'using namespace HeuristicCalculators;' here, members are accessed via this-> or explicitly.

SCPHeuristicCalculator::SCPHeuristicCalculator(Level& level)
    : HeuristicCalculators::HeuristicCalculator(level), // Explicitly call base constructor
      sic_sub_calculator_(level), 
      bgm_sub_calculator_(level) {}

int SCPHeuristicCalculator::calculateH(const State& state) {
    int total_h = 0;
    std::map<ActionType, int> saturated_costs; // Stores how much cost (0 or 1) is saturated for each action type
                                               // Initially all 0 (unsaturated)

    // Agent processing: Paths contribute to h_total and saturate action costs
    for (char agent_char : state.getAgentChars()) {
        // Define the current action cost function based on saturated_costs
        // An action costs 1 if not saturated, 0 if saturated.
        std::function<int(ActionType)> current_action_cost_fn = 
            [&saturated_costs](ActionType type) {
                if (saturated_costs.count(type) && saturated_costs[type] >= 1) {
                    return 0; // Cost is 0 if saturated
                }
                return 1; // Default cost is 1 if not saturated
            };

        std::pair<int, std::vector<const Action*>> path_info =
            this->sic_sub_calculator_.calculateSingleAgentPath(state, agent_char, current_action_cost_fn, this->currentLevel_);

        int agent_h_contribution = path_info.first;
        const std::vector<const Action*>& agent_path = path_info.second;

        if (agent_h_contribution == HeuristicCalculators::HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            // If any single agent cannot reach its goal even with potentially free actions,
            // the state is considered unsolvable by SCP.
            return HeuristicCalculators::HeuristicCalculator::MAX_HEURISTIC_VALUE;
        }
        
        total_h += agent_h_contribution; // Add the cost of this agent's path (using current available costs)

        // Saturate the costs of actions used in this agent's path
        // If an action was on the path, its original cost (1) is considered saturated.
        for (const Action* action_on_path : agent_path) {
            if (action_on_path != nullptr) { // Should always be true if path is valid
                 // Mark this action type as saturated (cost 1 consumed)
                 // If it was already saturated, this has no further effect on saturated_costs[type]
                 // but current_action_cost_fn would have made it free for this BFS.
                saturated_costs[action_on_path->type] = 1;
            }
        }
    }

    // Box processing (simplified for this version): 
    // Manhattan distance, does not use or saturate action costs.
    // Its contribution is simply added.
    for (char box_char : state.getBoxChars()) {
        int box_h_contribution = 
            this->bgm_sub_calculator_.calculateSingleBoxManhattanDistance(state, box_char, this->currentLevel_);
        
        if (box_h_contribution == HeuristicCalculators::HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            // This case implies box not found by calculateSingleBoxManhattanDistance, should be rare if getBoxChars is correct.
            // Or, if we adapt it to return MAX_H for other reasons (e.g. truly unreachable goal for a box by any means)
            return HeuristicCalculators::HeuristicCalculator::MAX_HEURISTIC_VALUE;
        }
        // If a box has no goal, calculateSingleBoxManhattanDistance returns 0, which is fine.
        total_h += box_h_contribution;
    }
    
    return total_h;
} 