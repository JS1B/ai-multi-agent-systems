#include "ConflictBasedSearcher.hpp"
#include <algorithm> // For std::sort, std::transform, etc. if needed
#include <limits>    // For std::numeric_limits if needed for max path length

ConflictBasedSearcher::ConflictBasedSearcher(const Level& initial_level_state)
    : level_data_(initial_level_state),
      low_level_searcher_(initial_level_state)
{
    // agents_.clear(); // Ensure clean state if constructor called multiple times (not typical)
    // agent_goals_.clear();

    // Populate agents_ vector by copying Agent objects from the Level's agentsMap
    // This also ensures we have their correct initial positions as stored in the Agent objects.
    for (const auto& map_pair : initial_level_state.agentsMap) {
        // map_pair.first is char agent_id
        // map_pair.second is const Agent& agent_object
        agents_.push_back(map_pair.second); // Copy the Agent object
        
        // Get the goal position for this agent
        // Assuming agent goals are indexed by the agent's character ID in goalsMap_for_Point2D_
        // or accessible via getGoalPosition. Let's use getGoalPosition for clarity.
        // Note: The Level class has getGoalPosition(char entity_id). If agent goals are stored
        // under their own ID (e.g., '0' for agent '0'), this is fine.
        // If agent goals are tied to box goals (e.g. agent '0' goal is where box 'A' should go, 
        // and Goal has a letter, then this logic needs adjustment based on problem spec)
        // For now, assume agent char ID directly maps to its goal in getGoalPosition.
        try {
             agent_goals_[map_pair.first] = initial_level_state.getGoalPosition(map_pair.first);
        } catch (const std::out_of_range& oor) {
            fprintf(stderr, "Warning: Could not find goal position for agent %c in Level::getGoalPosition. Out of range: %s\n", map_pair.first, oor.what());
            // Handle error: maybe throw, or assign a dummy goal, or skip agent?
            // For now, this agent might not have a goal in agent_goals_.
            // The find_solution loop later uses .at(agent_id) which would throw if key not present.
            // It's better if all agents considered have a goal.
            // Alternative: Check if initial_level_state.goalsMap_for_Point2D_.count(map_pair.first)
            // if (initial_level_state.goalsMap_for_Point2D_.count(map_pair.first)) {
            //    agent_goals_[map_pair.first] = initial_level_state.goalsMap_for_Point2D_.at(map_pair.first);
            // } else {
            //    fprintf(stderr, "Warning: No goal explicitly defined for agent %c in goalsMap_for_Point2D_.\n", map_pair.first);
            // }
        }
    }
    
    // Optional: Sort agents_ by ID if consistent iteration order is ever needed for other parts.
    // std::sort(agents_.begin(), agents_.end(), [](const Agent& a, const Agent& b){ return a.getId() < b.getId(); });

    // The old placeholder loop is removed:
    // for (const auto& pair : initial_level_state.initial_agent_positions) { ... }
}

int ConflictBasedSearcher::calculate_sum_of_costs(const SolutionPaths& solution) const {
    int total_cost = 0;
    for (const auto& pair : solution) {
        // AgentPath is std::vector<PathEntry>. PathEntry is {Point2D loc, int time}.
        // Cost of a path is its length (number of actions) which is size() - 1.
        // If path is empty or just one state (no moves), cost is 0.
        if (!pair.second.empty()) {
            total_cost += (pair.second.size() -1 );
        }
    }
    return total_cost;
}

std::vector<Constraint> ConflictBasedSearcher::get_constraints_for_agent(
    char agent_id, 
    const std::vector<Constraint>& all_constraints) const 
{
    std::vector<Constraint> agent_specific_constraints;
    for (const auto& constraint : all_constraints) {
        if (constraint.agent_id == agent_id) {
            agent_specific_constraints.push_back(constraint);
        }
    }
    return agent_specific_constraints;
}

std::optional<Point2D> ConflictBasedSearcher::get_location_at_time(const AgentPath& path, int time) const {
    if (time < 0) return std::nullopt;

    // A path is a sequence of (location, time_at_location) states.
    // If agent waits, multiple PathEntry might have same location but different times.
    // Or, more simply, path[t] gives location at time t if path is dense.
    // The AgentPath from LowLevelSearcher is: {pos_at_time_0, time_0}, {pos_at_time_1, time_1}, ...
    
    for(const auto& path_entry : path) {
        if (path_entry.second == time) {
            return path_entry.first;
        }
    }
    
    // If time is beyond the path's recorded duration, assume agent stays at its last known position.
    // This is a common convention in MAPF for conflict detection.
    if (!path.empty() && time >= path.back().second) {
        return path.back().first;
    }

    return std::nullopt; // Time not covered by path and not beyond last known point.
}

// --- Main find_conflicts method --- (Implementation will be complex)
bool ConflictBasedSearcher::find_conflicts(const SolutionPaths& current_solution, std::vector<CBSConflict>& conflicts_list) const {
    conflicts_list.clear();
    int max_time = 0;
    for (const auto& pair : current_solution) {
        if (!pair.second.empty()) {
            max_time = std::max(max_time, pair.second.back().second);
        }
    }

    // Iterate up to max_time encountered in any path
    for (int t = 0; t <= max_time; ++t) {
        // Check for vertex conflicts at time t
        emhash8::HashMap<Point2D, std::vector<char>, std::hash<Point2D>> locations_at_time_t;
        for (const auto& sol_pair : current_solution) {
            char agent_id = sol_pair.first;
            const AgentPath& path = sol_pair.second;
            std::optional<Point2D> loc_opt = get_location_at_time(path, t);
            if (loc_opt.has_value()) {
                locations_at_time_t[loc_opt.value()].push_back(agent_id);
            }
        }

        for (const auto& loc_pair : locations_at_time_t) {
            const std::vector<char>& agents_at_loc = loc_pair.second;
            if (agents_at_loc.size() > 1) {
                // Vertex conflict found
                for (size_t i = 0; i < agents_at_loc.size(); ++i) {
                    for (size_t j = i + 1; j < agents_at_loc.size(); ++j) {
                        conflicts_list.emplace_back(agents_at_loc[i], agents_at_loc[j], loc_pair.first, t, ConflictType::VERTEX);
                        // For basic CBS, often only the first conflict is taken and resolved.
                        // If we need all conflicts, this loop is fine. For now, let's assume we might return early.
                        // For simplicity here, we collect all and let the caller pick one.
                    }
                }
            }
        }

        // Check for edge conflicts (swapping) at time t (agents move from t to t+1)
        // Agent A: locA_t -> locA_t1 (at time t and t+1 respectively)
        // Agent B: locB_t -> locB_t1
        // Conflict if locA_t == locB_t1 AND locA_t1 == locB_t
        if (t < max_time) { // Edge conflicts occur for moves from t to t+1
            for (auto it1 = current_solution.begin(); it1 != current_solution.end(); ++it1) {
                char agent1_id = it1->first;
                const AgentPath& path1 = it1->second;
                std::optional<Point2D> loc1_t = get_location_at_time(path1, t);
                std::optional<Point2D> loc1_t1 = get_location_at_time(path1, t + 1);

                if (!loc1_t.has_value() || !loc1_t1.has_value()) continue;
                if (loc1_t.value() == loc1_t1.value()) continue; // Agent 1 waited, no edge traversed

                for (auto it2 = std::next(it1); it2 != current_solution.end(); ++it2) {
                    char agent2_id = it2->first;
                    const AgentPath& path2 = it2->second;
                    std::optional<Point2D> loc2_t = get_location_at_time(path2, t);
                    std::optional<Point2D> loc2_t1 = get_location_at_time(path2, t + 1);

                    if (!loc2_t.has_value() || !loc2_t1.has_value()) continue;
                    if (loc2_t.value() == loc2_t1.value()) continue; // Agent 2 waited

                    if (loc1_t.value() == loc2_t1.value() && loc1_t1.value() == loc2_t.value()) {
                        // Edge conflict (swapping)
                        // The CBSConflict constructor for EDGE is (id1, id2, loc_of_agent1_at_t, loc_of_agent2_at_t, time_of_arrival_at_swapped_locs)
                        conflicts_list.emplace_back(agent1_id, agent2_id, loc1_t.value(), loc2_t.value(), t + 1); // Corrected 5-arg call
                    }
                }
            }
        }
    }
    return !conflicts_list.empty();
}


// --- Main find_solution method --- (To be implemented next)
std::pair<SolutionPaths, int> ConflictBasedSearcher::find_solution(
    int max_hl_nodes_to_expand,
    int low_level_time_limit,
    int low_level_node_limit)
{
    // High-level OPEN list (priority queue of nodes to expand)
    std::priority_queue<std::shared_ptr<HighLevelNode>, 
                          std::vector<std::shared_ptr<HighLevelNode>>, 
                          CompareHighLevelNodes> hl_open_list;

    // Create the root node of the Constraint Tree
    auto root_node = std::make_shared<HighLevelNode>();
    root_node->constraints = {}; // Initially no constraints
    root_node->sum_of_costs = 0;

    // Plan initial paths for all agents with no constraints
    for (const auto& agent : agents_) {
        char agent_id = agent.getId();
        Point2D goal_pos = agent_goals_.at(agent_id);
        std::vector<Constraint> agent_constraints = get_constraints_for_agent(agent_id, root_node->constraints);
        
        auto [path, cost] = low_level_searcher_.find_path(agent, goal_pos, agent_constraints, low_level_time_limit, low_level_node_limit);
        
        if (cost == -1) { // No path found for an agent even with no constraints
            fprintf(stderr, "CBS Error: No initial path found for agent %c\n", agent_id);
            return {{}, -1}; // Cannot find a solution
        }
        root_node->solution[agent_id] = path;
        root_node->sum_of_costs += cost;
    }

    hl_open_list.push(root_node);
    int hl_nodes_expanded = 0;

    while(!hl_open_list.empty()) {
        if (hl_nodes_expanded >= max_hl_nodes_to_expand) {
            fprintf(stderr, "CBS Warning: Max high-level nodes expanded (%d), stopping.\n", max_hl_nodes_to_expand);
            return {{}, -1}; // Limit reached
        }

        std::shared_ptr<HighLevelNode> current_hl_node = hl_open_list.top();
        hl_open_list.pop();
        hl_nodes_expanded++;

        std::vector<CBSConflict> conflicts;
        if (!find_conflicts(current_hl_node->solution, conflicts)) {
            // No conflicts found, this is a valid solution!
            fprintf(stdout, "Solution found! Cost: %d, HL Nodes: %d\n", current_hl_node->sum_of_costs, hl_nodes_expanded);
            return {current_hl_node->solution, current_hl_node->sum_of_costs};
        }

        // At least one conflict exists. Pick one to resolve (e.g., the first one found).
        // More sophisticated conflict selection can be implemented here (e.g. cardinal, semi-cardinal conflicts)
        const CBSConflict& conflict_to_resolve = conflicts.front(); 

        // Create two child nodes by adding constraints for each agent involved in the conflict
        auto [constraint1, constraint2] = conflict_to_resolve.get_constraints();

        for (const auto* chosen_constraint : {&constraint1, &constraint2}) {
            auto next_hl_node = std::make_shared<HighLevelNode>();
            next_hl_node->constraints = current_hl_node->constraints; // Inherit parent's constraints
            next_hl_node->constraints.push_back(*chosen_constraint);  // Add the new constraint
            
            // Re-plan for the agent affected by the new constraint
            // And copy paths for other agents from the parent node initially
            next_hl_node->solution = current_hl_node->solution;
            char agent_to_replan = chosen_constraint->agent_id;
            const Agent* agent_obj = nullptr;
            for(const auto& ag : agents_) { if(ag.getId() == agent_to_replan) {agent_obj = &ag; break;}}
            if (!agent_obj) { 
                 fprintf(stderr, "CBS Error: Agent %c not found for replanning.\n", agent_to_replan);
                 continue; // Should not happen
            }

            Point2D goal_pos = agent_goals_.at(agent_to_replan);
            std::vector<Constraint> agent_specific_constraints = get_constraints_for_agent(agent_to_replan, next_hl_node->constraints);

            auto [new_path, new_cost] = low_level_searcher_.find_path(*agent_obj, goal_pos, agent_specific_constraints, low_level_time_limit, low_level_node_limit);

            if (new_cost == -1) { // Agent cannot find path with new constraint
                continue; // This branch of CT is pruned (dead end)
            }
            
            next_hl_node->solution[agent_to_replan] = new_path;
            
            // Recalculate sum of costs for the new node
            // (Cost of old path for this agent - Cost of new path for this agent) + Parent_SOC
            // OR, more robustly, recalculate from scratch if other agents might be affected by future heuristics (not the case for basic CBS)
            next_hl_node->sum_of_costs = 0; // Recalculate fully
            for(const auto& sol_pair : next_hl_node->solution) {
                if(!sol_pair.second.empty()) {
                    next_hl_node->sum_of_costs += (sol_pair.second.size() -1);
                }
            }
            // An optimization: next_hl_node->sum_of_costs = current_hl_node->sum_of_costs - (cost_of_old_path_for_replan_agent) + new_cost;
            // This requires storing old cost or accessing it.

            hl_open_list.push(next_hl_node);
        }
    }

    fprintf(stderr, "CBS Warning: High-level OPEN list empty, no solution found.\n");
    return {{}, -1}; // No solution found
} 