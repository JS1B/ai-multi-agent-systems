#ifndef SINGLEBOXPDBHEURISTICCALCULATOR_HPP
#define SINGLEBOXPDBHEURISTICCALCULATOR_HPP

#include "HeuristicCalculator.hpp"
#include "level.hpp"
#include "state.hpp"
#include "action.hpp" // For ActionType
#include <vector>
#include <string>
#include <map> // For precomputed_paths_ if using map from Point2D

namespace HeuristicCalculators {

class SingleBoxPDBHeuristicCalculator : public HeuristicCalculator {
public:
    SingleBoxPDBHeuristicCalculator(const Level& level, char target_box_id, char target_goal_id);
    ~SingleBoxPDBHeuristicCalculator() override = default;

    int calculateH(const State& state, const ActionCostMap& costs_for_this_heuristic) const override;
    ActionCostMap saturate(const State& state, const ActionCostMap& available_costs) const override;
    std::string getName() const override;

private:
    const Level& level_ref_; // Reference to the level for geometry
    char target_box_id_;
    char target_goal_id_;
    Point2D box_goal_pos_; // Store the goal position for easy access

    // PDB: Maps from a box's position (Point2D) to a sequence of push actions to get to its goal.
    // Using a map for sparse PDBs, or could be a 2D vector if all cells are computed.
    std::map<Point2D, std::vector<ActionType>> precomputed_push_sequences_;

    // Helper for PDB generation
    void generate_pdb(); 
};

} // namespace HeuristicCalculators

#endif // SINGLEBOXPDBHEURISTICCALCULATOR_HPP 