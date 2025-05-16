#ifndef SCPHEURISTICCALCULATOR_HPP
#define SCPHEURISTICCALCULATOR_HPP

#include "HeuristicCalculator.hpp"
// Forward declarations for sub-calculator types, or include their headers if absolutely needed for unique_ptr deleter
// #include "SumOfIndividualCostsCalculator.hpp"
// #include "BoxGoalManhattanDistanceCalculator.hpp"
// #include "SingleBoxPDBHeuristicCalculator.hpp"
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
// #include <functional> // No longer needed for std::reference_wrapper

namespace HeuristicCalculators {

// Forward declare base HeuristicCalculator if not fully included by others.
// class HeuristicCalculator; // Already in HeuristicCalculator.hpp

class SCPHeuristicCalculator : public HeuristicCalculator {
public:
    // Constructor takes ownership of an ordered list of sub-heuristic calculators.
    SCPHeuristicCalculator(const Level& level_ref,
                         std::vector<std::unique_ptr<HeuristicCalculator>> ordered_sub_calculators_to_own);
    
    ~SCPHeuristicCalculator() override = default;

    int calculateH(const State& state, const ActionCostMap& initial_total_costs) const override;
    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override;
    std::string getName() const override { return "scp"; } 

private:
    // SCP owns its sub-calculators
    std::vector<std::unique_ptr<HeuristicCalculator>> owned_ordered_sub_calculators_;
    // No specific agent/box IDs needed here if sub-calculators handle their own scope.
};

} // namespace HeuristicCalculators

#endif // SCPHEURISTICCALCULATOR_HPP