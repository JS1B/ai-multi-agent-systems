#include "ConflictBasedSearcher.hpp"
#include "level.hpp" // For Point2D, Level, Agent, etc.
#include <iostream> // For std::cerr
#include <algorithm> // For std::sort, std::max

ConflictBasedSearcher::ConflictBasedSearcher(const Level& level)
    : low_level_searcher_(level) // Initialize low_level_searcher_ with the level
{
    // Populate agents_ and agent_goals_ from the Level object
    for (const auto& agent_char_entry : level.agentsInitialPositions) {
        char agent_id = agent_char_entry.first;
        Point2D start_pos = agent_char_entry.second;
        Point2D goal_pos = level.getGoalPosition(agent_id);

        if (goal_pos != Point2D{-1,-1}) { // Check if a valid goal exists
            agents_.emplace_back(agent_id, start_pos, goal_pos); // Assuming Agent constructor (char id, Point2D start, Point2D goal)
            agent_goals_[agent_id] = goal_pos;
        } else {
            std::cerr << "Warning: Goal not found for agent " << agent_id 
                      << " or agent does not exist in goal map." << std::endl;
            // Decide how to handle this: throw error, or skip agent?
            // For now, skipping agent if goal is not defined.
        }
    }
    // Ensure all agents in agentsMap also have goals and are added if not already by initialPositions
    for (const auto& agent_map_entry : level.agentsMap) {
        char agent_id = agent_map_entry.first;
        if (agent_goals_.find(agent_id) == agent_goals_.end()) { // If not already processed
            Point2D start_pos = agent_map_entry.second;
            Point2D goal_pos = level.getGoalPosition(agent_id);
            if (goal_pos != Point2D{-1,-1}) {
                 agents_.emplace_back(agent_id, start_pos, goal_pos);
                 agent_goals_[agent_id] = goal_pos;
            } else {
                 std::cerr << "Warning: Goal not found for agent " << agent_id 
                           << " from agentsMap." << std::endl;
            }
        }
    }
}

SolutionPaths ConflictBasedSearcher::find_solution() {
    std::priority_queue<std::shared_ptr<HighLevelNode>,
                          std::vector<std::shared_ptr<HighLevelNode>>,
                          CompareHighLevelNodes> open_set;

    // Create root node
    auto root = std::make_shared<HighLevelNode>();
    root->sum_of_costs = 0;

    // Plan initial paths for all agents
    for (const auto& agent : agents_) {
        std::vector<Constraint> agent_constraints = get_constraints_for_agent(agent.getId(), root->constraints);
        Point2D goal_pos = agent_goals_.at(agent.getId());
        auto [path, cost] = low_level_searcher_.find_path(agent, goal_pos, agent_constraints);

        if (cost == -1) {
            std::cerr << "CBS: No initial path found for agent " << agent.getId() << std::endl;
            return {}; // No solution if any agent cannot find an initial path
        }
        root->solution[agent.getId()] = path;
        root->sum_of_costs += cost;
    }

    open_set.push(root);

    int iterations = 0;
    const int MAX_ITERATIONS = 10000; // Safety break

    while (!open_set.empty() && iterations < MAX_ITERATIONS) {
        iterations++;
        std::shared_ptr<HighLevelNode> current_node = open_set.top();
        open_set.pop();

        std::vector<CBSConflict> conflicts = find_conflicts(current_node->solution);

        if (conflicts.empty()) {
            std::cout << "CBS: Solution found after " << iterations << " iterations." << std::endl;
            return current_node->solution; // Solution found
        }

        // For simplicity, pick the first conflict
        // A more advanced strategy might pick the earliest or "best" conflict to resolve
        CBSConflict conflict_to_resolve = conflicts.front(); 

        std::vector<Constraint> new_constraints = conflict_to_resolve.get_constraints();

        for (const auto& new_constraint : new_constraints) {
            auto child_node = std::make_shared<HighLevelNode>();
            child_node->constraints = current_node->constraints; // Copy parent constraints
            child_node->constraints.push_back(new_constraint);   // Add new constraint
            child_node->solution = current_node->solution;       // Copy parent solution initially

            // Agent affected by the new constraint
            char agent_to_replan = new_constraint.agent_id;
            const Agent* agent_obj = nullptr;
            for(const auto& ag : agents_){
                if(ag.getId() == agent_to_replan) {
                    agent_obj = &ag;
                    break;
                }
            }
            if (!agent_obj) {
                 std::cerr << "Error: Agent object not found for ID " << agent_to_replan << std::endl;
                 continue; // Skip this child
            }

            std::vector<Constraint> agent_specific_constraints = get_constraints_for_agent(agent_to_replan, child_node->constraints);
            Point2D goal_pos = agent_goals_.at(agent_to_replan);
            
            auto [new_path, new_cost] = low_level_searcher_.find_path(*agent_obj, goal_pos, agent_specific_constraints);

            if (new_cost == -1) {
                // std::cerr << "CBS Child: No path found for agent " << agent_to_replan << " with new constraint." << std::endl;
                continue; // This branch is infeasible
            }

            child_node->solution[agent_to_replan] = new_path;
            child_node->sum_of_costs = calculate_sum_of_costs(child_node->solution); // Recalculate total cost
            
            open_set.push(child_node);
        }
    }
    if(iterations >= MAX_ITERATIONS){
        std::cerr << "CBS: Reached max iterations, no solution found." << std::endl;
    }
    else {
        std::cerr << "CBS: Open set empty, no solution found." << std::endl;
    }
    return {}; // No solution found within limits
}

int ConflictBasedSearcher::calculate_sum_of_costs(const SolutionPaths& solution) const {
    int total_cost = 0;
    for (const auto& pair : solution) {
        // The cost is path length minus 1, but low-level returns path length (g-cost)
        // If path is empty (e.g. agent already at goal), its length might be 1 (just the goal state at t=0)
        // Or, more simply, the g-cost returned by low-level is the number of actions.
        if (!pair.second.empty()) {
             // Cost is the time of the last entry if path stores (pos, time) and represents states visited
             // Or if low-level searcher returns cost directly, use that.
             // Assuming path length is number of states, so actions = length - 1.
             // However, our low_level_searcher.find_path returns cost which is g value (path length/actions).
             // Let's assume the cost is simply the size of the path vector if it represents moves,
             // or the g-cost from low-level. find_path returns {path, cost} where cost is g.
             // So we sum these g-costs.
            
            // Find the agent in agents_ to get its original cost if path is empty.
            // This function is called after a path is found, so path.second is AgentPath.
            // The g_cost of the last state in path is path.back().time IF g == time. Not necessarily.
            // The low_level_searcher returns the cost explicitly. Summing these costs is done when node is created/updated.
            // This function should re-calculate based on the paths stored.
            total_cost += pair.second.size() > 0 ? static_cast<int>(pair.second.size() -1) : 0;
        } // if agent stands still at goal, path has 1 entry, cost is 0.
    }
    return total_cost;
}


std::vector<Constraint> ConflictBasedSearcher::get_constraints_for_agent(
    char agent_id,
    const std::vector<Constraint>& all_constraints) const {
    std::vector<Constraint> agent_specific_constraints;
    for (const auto& c : all_constraints) {
        if (c.agent_id == agent_id) {
            agent_specific_constraints.push_back(c);
        }
    }
    return agent_specific_constraints;
}

Point2D ConflictBasedSearcher::get_location_at_time(const AgentPath& path, int time) const {
    if (path.empty()) {
        return {-1, -1}; // Invalid or unknown location
    }
    // If time is beyond path, assume agent stays at its last location
    if (time >= static_cast<int>(path.size())) {
        return path.back().first; // .first is Point2D
    }
    // Path entries are (Point2D location, int time_step). Find matching time_step.
    // Assuming path is dense, i.e., path[t] is the state at time t.
    // If path is (location, time_at_location), need to search or interpolate.
    // Given AgentPath = std::vector<std::pair<Point2D, int>>, and assuming pair.second is the time step.
    for(const auto& p_entry : path) {
        if (p_entry.second == time) {
            return p_entry.first;
        }
    }
    // If exact time not found but path is shorter, agent might be at its goal.
    // Or, if time is within recorded path states but not an exact match (sparse path). This needs clarification.
    // For space-time A*, path[t] should be location at time t.
    // If path is std::vector<PathEntry> where PathEntry is pair<Point2D, int time_step>, and path is ordered by time_step
    // AND path[i].second == i (for dense path), then path[time].first is the location.
    if (time < static_cast<int>(path.size()) && path[time].second == time) { // Check if path[time] is for current time.
         return path[time].first;
    }
    // Fallback: if time is beyond the explicitly stored path, assume agent stays at the last known position.
    if (!path.empty() && time >= path.back().second) {
        return path.back().first;
    }

    return {-1, -1}; // Placeholder: indicates location not determined
}

std::vector<CBSConflict> ConflictBasedSearcher::find_conflicts(const SolutionPaths& current_solution) const {
    std::vector<CBSConflict> conflicts;
    if (current_solution.empty()) return conflicts;

    // Determine max_time to check for conflicts
    int max_time = 0;
    for (const auto& pair : current_solution) {
        if (!pair.second.empty()) {
            // PathEntry is std::pair<Point2D, int time_step>
            max_time = std::max(max_time, pair.second.back().second);
        }
    }

    // Check for vertex conflicts
    for (int t = 0; t <= max_time; ++t) {
        emhash8::HashMap<Point2D, std::vector<char>, std::hash<Point2D>> locations_at_time_t;
        for (const auto& sol_pair : current_solution) {
            char agent_id = sol_pair.first;
            const AgentPath& path = sol_pair.second;
            Point2D loc = get_location_at_time(path, t);
            if (loc != Point2D{-1, -1}) { // If agent has a valid location at this time
                locations_at_time_t[loc].push_back(agent_id);
            }
        }
        for (const auto& loc_entry : locations_at_time_t) {
            if (loc_entry.second.size() > 1) { // More than one agent at the same location and time
                // Found a vertex conflict
                // For simplicity, report conflict between first two agents found
                conflicts.emplace_back(loc_entry.second[0], loc_entry.second[1], loc_entry.first, t, ConflictType::VERTEX);
            }
        }
    }

    // Check for edge conflicts (swapping places)
    for (const auto& sol_pair1 : current_solution) {
        char agent1_id = sol_pair1.first;
        const AgentPath& path1 = sol_pair1.second;

        for (const auto& sol_pair2 : current_solution) {
            char agent2_id = sol_pair2.first;
            if (agent1_id >= agent2_id) continue; // Avoid duplicate pairs and self-conflict check
            const AgentPath& path2 = sol_pair2.second;

            int min_path_len = std::min(path1.empty() ? 0 : path1.back().second,
                                        path2.empty() ? 0 : path2.back().second);
            
            for (int t = 0; t < min_path_len; ++t) {
                Point2D loc1_t = get_location_at_time(path1, t);
                Point2D loc1_t_plus_1 = get_location_at_time(path1, t + 1);
                Point2D loc2_t = get_location_at_time(path2, t);
                Point2D loc2_t_plus_1 = get_location_at_time(path2, t + 1);

                if (loc1_t != Point2D{-1,-1} && loc1_t_plus_1 != Point2D{-1,-1} && 
                    loc2_t != Point2D{-1,-1} && loc2_t_plus_1 != Point2D{-1,-1}) {
                    if (loc1_t_plus_1 == loc2_t && loc2_t_plus_1 == loc1_t) {
                        // Edge conflict: A1 at L1->L2, A2 at L2->L1
                        conflicts.emplace_back(agent1_id, agent2_id, loc1_t, loc1_t_plus_1, t + 1, ConflictType::EDGE);
                    }
                }
            }
        }
    }
    return conflicts;
} 