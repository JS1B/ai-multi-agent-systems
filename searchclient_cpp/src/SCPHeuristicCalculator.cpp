#include "SCPHeuristicCalculator.hpp"
// #include "common.hpp" // Removed non-existent common.hpp
#include <algorithm> // For std::max, std::find_if
#include "HeuristicCalculator.hpp" // Ensure base class definition is available

// Note: HeuristicCalculator.hpp (included by SCPHeuristicCalculator.hpp) brings in the HeuristicCalculators namespace
// so we don't need 'using namespace HeuristicCalculators;' here, members are accessed via this-> or explicitly.

namespace HeuristicCalculators {

SCPHeuristicCalculator::SCPHeuristicCalculator(const Level& level_ref,
                                           std::vector<std::unique_ptr<HeuristicCalculator>> ordered_sub_calculators_to_own)
    : HeuristicCalculator(level_ref), // Call base constructor
      owned_ordered_sub_calculators_(std::move(ordered_sub_calculators_to_own)) {
    // Constructor body: member is initialized by move
}

int SCPHeuristicCalculator::calculateH(const State& state, const ActionCostMap& initial_total_costs) const {
    int total_h_value = 0;
    ActionCostMap current_remaining_budget = initial_total_costs;

    for (const auto& sub_calculator_ptr : owned_ordered_sub_calculators_) {
        if (!sub_calculator_ptr) continue; // Should not happen if vector is managed well
        HeuristicCalculator& sub_calculator = *sub_calculator_ptr;

        ActionCostMap consumed_by_sub = sub_calculator.saturate(state, current_remaining_budget);
        
        // The sub-calculator's h-value should be based on the costs *it claims it can use*,
        // which are effectively represented by 'consumed_by_sub' in terms of cost values,
        // or rather, it should be based on the current_remaining_budget to see what it *can* achieve.
        // Let's refine this. The sub-calculator.calculateH should take the current_remaining_budget.
        // And its saturate method already told us what it would consume from that budget.
        // The h_contribution should be calculated based on what it *would* consume from the current budget.
        // The previous SCP impl passed consumed_by_sub to calculateH, which seems more correct for SCP.
        // calculateH(state, budget_for_this_calc)
        // Let's assume sub_calculator.calculateH is designed to take the budget it *can* use.
        // The crucial part is that `consumed_by_sub` tells us what it *would* take from `current_remaining_budget`.
        // If a sub-heuristic (like SIC or PDB) calculates its h-value from its `saturate` output 
        // (i.e., the costs it identified it needs), that value should be correct.
        // However, the traditional HeuristicCalculator::calculateH takes a general ActionCostMap.
        // For SCP, the budget passed to calculateH should be the one *after* this sub_calculator has notionally paid.
        // No, that's not right. It should be calculated with the *currently available costs*.
        // The `consumed_by_sub` is what it *would* consume. The `h_contribution` is its estimate.

        // The sub-calculator should calculate its H value based on the costs *it was given* (current_remaining_budget)
        // and its saturate() method determined what portion of those costs it would ideally consume.
        // The h_value should be calculated based on `current_remaining_budget` before it's reduced by `consumed_by_sub`.
        // However, the original SCP logic in this file did: h_contribution = sic_sub_calculator_.calculateH(state, consumed_by_sub);
        // This means the sub-heuristic calculated its h-value based on the costs it *actually consumed*.
        // This seems more aligned with the idea of cost partitioning.
        // Let's stick to: calculateH for a sub-heuristic takes the cost map representing what it *did* consume.
        int h_contribution = sub_calculator.calculateH(state, consumed_by_sub); 

        if (h_contribution == HeuristicCalculator::MAX_HEURISTIC_VALUE) {
            return HeuristicCalculator::MAX_HEURISTIC_VALUE;
        }
        total_h_value += h_contribution;
        current_remaining_budget = HeuristicCalculators::subtractCosts(current_remaining_budget, consumed_by_sub);
    }

    return total_h_value;
}

ActionCostMap SCPHeuristicCalculator::saturate(const State& state, const ActionCostMap& available_costs) const {
    ActionCostMap total_consumed_costs_by_scp;
    ActionCostMap current_remaining_budget = available_costs;

    for (const auto& sub_calculator_ptr : owned_ordered_sub_calculators_) {
        if (!sub_calculator_ptr) continue;
        HeuristicCalculator& sub_calculator = *sub_calculator_ptr;

        ActionCostMap consumed_by_this_sub = sub_calculator.saturate(state, current_remaining_budget);

        for (const auto& pair : consumed_by_this_sub) {
            total_consumed_costs_by_scp[pair.first] += pair.second;
        }
        current_remaining_budget = HeuristicCalculators::subtractCosts(current_remaining_budget, consumed_by_this_sub);
    }
    return total_consumed_costs_by_scp;
}

} // namespace HeuristicCalculators

// ... any other helper methods, ensure they use this->level_ ... 