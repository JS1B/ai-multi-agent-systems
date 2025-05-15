#ifndef SCPHEURISTICCALCULATOR_HPP
#define SCPHEURISTICCALCULATOR_HPP

#include "HeuristicCalculator.hpp"
#include "level.hpp"
#include "state.hpp"
#include <vector>
#include <map>
#include <memory>
#include <functional>

class SCPHeuristicCalculator : public HeuristicCalculators::HeuristicCalculator {
public:
    SCPHeuristicCalculator(Level& level);
    ~SCPHeuristicCalculator() override = default;

    int calculateH(const State& state) override;
    std::string getName() const override { return "scp"; }

private:
    // Individual heuristic calculators for agents (SIC-like) and boxes (BGM-like)
    // These will be used to get the heuristic for individual entities.
    HeuristicCalculators::SumOfIndividualCostsCalculator sic_sub_calculator_;
    HeuristicCalculators::BoxGoalManhattanDistanceCalculator bgm_sub_calculator_;
    
    // Note: currentLevel_ is inherited from HeuristicCalculator base class
    // which is HeuristicCalculators::HeuristicCalculator
};

#endif // SCPHEURISTICCALCULATOR_HPP 