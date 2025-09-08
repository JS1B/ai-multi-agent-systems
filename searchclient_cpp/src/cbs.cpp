#include "cbs.hpp"

// C++
#include <cassert>
#include <queue>
#include <unordered_map>

#include "memory.hpp"

void printSearchStatus(const CBSFrontier &cbs_frontier, const size_t &generated_states_count) {
    static bool first_time = true;

    if (first_time) {
        fprintf(stdout, "#CBS fr size, Mem usage[MB], Generated states\n");
        first_time = false;
    }

    fprintf(stdout, "#%11zu, %13d, %16zu\n", cbs_frontier.size(), Memory::getUsage(), generated_states_count);
    fflush(stdout);
}

CBS::CBS(const Level &loaded_level) : initial_level(loaded_level) {
    // Group agents by color
    std::map<Color, std::vector<Agent>> agents_by_color;
    std::map<Color, std::vector<BoxBulk>> boxes_by_color;

    // Group agents by their color
    for (const auto &agent : loaded_level.agents) {
        Color agent_color = initial_level.static_level.getAgentColor(agent.getSymbol());
        agents_by_color[agent_color].push_back(agent);
    }

    // Group boxes by their color
    for (const auto &box : loaded_level.boxes) {
        boxes_by_color[box.getColor()].push_back(box);
    }

    // Create LowLevelState for each color group that has agents
    for (const auto &[color, agents] : agents_by_color) {
        std::vector<BoxBulk> matching_boxes;
        if (boxes_by_color.find(color) != boxes_by_color.end()) {
            // Copy BoxBulk objects individually since assignment operator is deleted
            for (const auto &box : boxes_by_color[color]) {
                matching_boxes.push_back(BoxBulk(box.getPositions(), box.getGoals(), box.getColor(), box.getSymbol()));
            }
        }

        initial_agents_states_.push_back(new LowLevelState(initial_level.static_level, agents, matching_boxes));
    }

    agents_num_ = initial_agents_states_.size();

    // Build agent symbol mapping using helper function
    auto mapping_result = buildAgentMapping(initial_agents_states_);
    agent_symbol_to_group_info_ = std::move(mapping_result.first);
    total_agents_ = mapping_result.second;
}

CBS::~CBS() {
    for (auto agent_state : initial_agents_states_) {
        delete agent_state;
    }
}

std::vector<std::vector<const Action *>> CBS::solve() {
    size_t generated_states_count = 0;
    CTNode root;

    CBSFrontier cbs_frontier;

    // root.constraints.reserve(agents_states_.size());
    // root.solutions.reserve(agents_states_.size());

    // Create AgentGraphSearch objects for each agent_state
    std::vector<Graphsearch *> agent_searches;
    agent_searches.reserve(initial_agents_states_.size());
    for (auto agent_state : initial_agents_states_) {
        agent_searches.push_back(new Graphsearch(agent_state, new FrontierBestFirst(new HeuristicAStar())));
    }

    // Find a solution for each agent bulk
    for (size_t i = 0; i < agent_searches.size(); i++) {
        auto bulk_plan = agent_searches[i]->solve({});
        generated_states_count += agent_searches[i]->getGeneratedStatesCount();
        if (!agent_searches[i]->wasSolutionFound()) {
            printSearchStatus(cbs_frontier, generated_states_count);
            return {};
        }
        root.solutions.push_back(bulk_plan);
    }

    root.cost = utils::CBS_cost(root.solutions);

    cbs_frontier.add(&root);

    size_t iterations = 0;
    while (!cbs_frontier.isEmpty()) {
        if (iterations < 5 || iterations % 20 == 0) {  // 10000
            printSearchStatus(cbs_frontier, generated_states_count);
        }

        CTNode *node = cbs_frontier.pop();

        // Check for duplicate constraint sets
        if (visited_constraint_sets_.find(node->one_sided_conflicts) != visited_constraint_sets_.end()) {
            delete node;
            continue;
        }
        visited_constraint_sets_.insert(node->one_sided_conflicts);

        if (Memory::getUsage() > Memory::maxUsage) {
            fprintf(stderr, "Maximum memory usage exceeded.\n");
            printSearchStatus(cbs_frontier, generated_states_count);
            return {};
        }

        std::vector<std::vector<const Action *>> merged_plans = mergePlans(node->solutions);
        FullConflict conflict = findFirstConflict(merged_plans);
        if (conflict.a1_symbol == 0 && conflict.a2_symbol == 0) {
            printSearchStatus(cbs_frontier, generated_states_count);
            return merged_plans;
        }

        // fprintf(stderr, "Conflict (%c %c) in (%i %i) at %lu\t", conflict.a1_symbol, conflict.a2_symbol, conflict.constraint.vertex.r,
        //         conflict.constraint.vertex.c, conflict.constraint.g);
        // fprintf(stderr, "One-sided conflicts: %zu, Node cost: %zu\n", node->one_sided_conflicts.size(), node->cost);
        // fflush(stderr);

        for (auto agent_symbol : {conflict.a1_symbol, conflict.a2_symbol}) {
            // Fast lookup of group index for this agent symbol
            auto it = agent_symbol_to_group_info_.find(agent_symbol);
            if (it == agent_symbol_to_group_info_.end()) {
                fprintf(stderr, "Error: Could not find group for agent symbol %c\n", agent_symbol);
                continue;
            }
            uint_fast8_t group_idx = it->second.first;

            CTNode *child = new CTNode(*node);

            auto osc_pair = conflict.split();
            if (osc_pair.first.a1_symbol == agent_symbol) {
                child->one_sided_conflicts.insert(osc_pair.first);
            }
            if (osc_pair.second.a1_symbol == agent_symbol) {
                child->one_sided_conflicts.insert(osc_pair.second);
            }

            // Get constraints from conflicts of an agent
            std::vector<Constraint> constraints;
            constraints.reserve(child->one_sided_conflicts.size());
            for (const auto &conflict : child->one_sided_conflicts) {
                if (conflict.a1_symbol == agent_symbol) {
                    constraints.push_back(conflict.constraint);
                }
            }

            Graphsearch *agent_search = agent_searches[group_idx];
            auto plan = agent_search->solve(constraints);
            generated_states_count += agent_search->getGeneratedStatesCount();
            child->solutions[group_idx] = plan;
            if (!agent_search->wasSolutionFound()) {
                child->cost = SIZE_MAX;
                delete child;
                continue;
            }
            child->cost = utils::CBS_cost(child->solutions);
            cbs_frontier.add(child);
        }

        iterations++;
    }
    printSearchStatus(cbs_frontier, generated_states_count);
    return {};
}

std::vector<std::vector<const Action *>> CBS::mergePlans(std::vector<std::vector<std::vector<const Action *>>> &plans) {
    auto plans_copy = plans;

    // Use the class member mapping (std::map keeps keys sorted by symbol)

    // Make all plans the same length
    size_t longest_plan_length = 0;
    for (size_t i = 0; i < plans_copy.size(); i++) {
        longest_plan_length = std::max(longest_plan_length, plans_copy[i].size());
    }

    // Extend plans to same length with NoOp
    for (size_t i = 0; i < plans_copy.size(); i++) {
        plans_copy[i].resize(longest_plan_length, std::vector<const Action *>(plans_copy[i][0].size(), (const Action *)&Action::NoOp));
    }

    std::vector<const Action *> row;
    row.reserve(total_agents_);
    std::vector<std::vector<const Action *>> merged_plans;
    merged_plans.reserve(longest_plan_length);

    // Merge plans in correct agent symbol order (map iteration is sorted by key)
    for (size_t depth = 0; depth < longest_plan_length; depth++) {
        row.clear();
        for (const auto &[symbol, group_info] : agent_symbol_to_group_info_) {
            uint_fast8_t group_idx = group_info.first;
            uint_fast8_t agent_idx = group_info.second;
            row.push_back(plans_copy[group_idx][depth][agent_idx]);
        }
        merged_plans.push_back(row);
    }

    return merged_plans;
}

// Helper function to find which agent is responsible for moving a specific box
size_t CBS::findAgentResponsibleForBox(size_t box_index, const std::vector<const Action *> &actions,
                                       const std::vector<Cell2D> &previous_agent_positions,
                                       const std::vector<Cell2D> &previous_box_positions) const {
    Cell2D box_previous_pos = previous_box_positions[box_index];

    for (size_t agent_idx = 0; agent_idx < actions.size(); ++agent_idx) {
        const Action *action = actions[agent_idx];
        Cell2D agent_pos = previous_agent_positions[agent_idx];

        if (action->type == ActionType::Push) {
            Cell2D expected_box_pos = agent_pos + action->agent_delta;
            if (expected_box_pos == box_previous_pos) {
                return agent_idx;
            }
        } else if (action->type == ActionType::Pull) {
            Cell2D expected_box_pos = agent_pos - action->box_delta;
            if (expected_box_pos == box_previous_pos) {
                return agent_idx;
            }
        }
    }

    return SIZE_MAX;  // No agent responsible (box didn't move)
}

FullConflict CBS::findFirstConflict(const std::vector<std::vector<const Action *>> &solutions) const {
    // assert(solutions[0].size() == agents_num_);

    std::vector<Cell2D> current_agent_positions(solutions[0].size());
    std::vector<Cell2D> previous_agent_positions(solutions[0].size());

    // Track all box positions as a flat list of Cell2D
    std::vector<Cell2D> current_box_positions;
    std::vector<Cell2D> previous_box_positions;

    // Initialize agent positions
    for (size_t i = 0; i < solutions[0].size(); i++) {
        current_agent_positions[i] = initial_level.agents[i].getPosition();
        previous_agent_positions[i] = initial_level.agents[i].getPosition();
    }

    // Initialize box positions from all box bulks
    for (const auto &box_bulk : initial_level.boxes) {
        for (size_t i = 0; i < box_bulk.size(); i++) {
            current_box_positions.push_back(box_bulk.getPosition(i));
        }
    }
    previous_box_positions = current_box_positions;

    for (size_t depth = 0; depth < solutions.size(); depth++) {
        // Update position history
        previous_agent_positions = current_agent_positions;
        previous_box_positions = current_box_positions;

        // Apply agent actions
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            current_agent_positions[j] = previous_agent_positions[j] + solutions[depth][j]->agent_delta;
        }

        // Apply box movements for push/pull actions
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            const Action *action = solutions[depth][j];
            Cell2D agent_pos = previous_agent_positions[j];

            if (action->type == ActionType::Push) {
                Cell2D box_initial_pos = agent_pos + action->agent_delta;
                Cell2D box_final_pos = box_initial_pos + action->box_delta;

                // Find and move the box
                for (size_t box_idx = 0; box_idx < current_box_positions.size(); ++box_idx) {
                    if (current_box_positions[box_idx] == box_initial_pos) {
                        current_box_positions[box_idx] = box_final_pos;
                        break;
                    }
                }
            } else if (action->type == ActionType::Pull) {
                Cell2D box_initial_pos = agent_pos - action->box_delta;
                Cell2D box_final_pos = agent_pos;  // Box moves to agent's current position

                // Find and move the box
                for (size_t box_idx = 0; box_idx < current_box_positions.size(); ++box_idx) {
                    if (current_box_positions[box_idx] == box_initial_pos) {
                        current_box_positions[box_idx] = box_final_pos;
                        break;
                    }
                }
            }
        }

        // 1. Agent-Agent Vertex conflicts
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            for (size_t k = j + 1; k < solutions[0].size(); ++k) {
                if (current_agent_positions[j] == current_agent_positions[k]) {
                    return FullConflict(j + FIRST_AGENT, k + FIRST_AGENT, Constraint(current_agent_positions[j], depth + 1));
                }
            }
        }

        // 2. Agent-Agent Follow/trailing conflicts
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            for (size_t k = j + 1; k < solutions[0].size(); ++k) {
                if (previous_agent_positions[k] == current_agent_positions[j]) {
                    return FullConflict(k + FIRST_AGENT, j + FIRST_AGENT, Constraint(current_agent_positions[j], depth + 1));
                }
                if (previous_agent_positions[j] == current_agent_positions[k]) {
                    return FullConflict(j + FIRST_AGENT, k + FIRST_AGENT, Constraint(current_agent_positions[k], depth + 1));
                }
            }
        }

        // 3. Box-Box Vertex conflicts
        for (size_t j = 0; j < current_box_positions.size(); ++j) {
            for (size_t k = j + 1; k < current_box_positions.size(); ++k) {
                if (current_box_positions[j] == current_box_positions[k]) {
                    // Return conflict between the agents that moved these boxes
                    // Note: We need to find which agents are responsible for these box movements
                    size_t agent_j = findAgentResponsibleForBox(j, solutions[depth], previous_agent_positions, previous_box_positions);
                    size_t agent_k = findAgentResponsibleForBox(k, solutions[depth], previous_agent_positions, previous_box_positions);
                    if (agent_j != SIZE_MAX && agent_k != SIZE_MAX && agent_j != agent_k) {
                        return FullConflict(agent_j + FIRST_AGENT, agent_k + FIRST_AGENT, Constraint(current_box_positions[j], depth + 1));
                    }
                }
            }
        }

        // 4. Box-Box Follow conflicts
        for (size_t j = 0; j < current_box_positions.size(); ++j) {
            for (size_t k = 0; k < current_box_positions.size(); ++k) {
                if (j != k && previous_box_positions[k] == current_box_positions[j] &&
                    previous_box_positions[j] == current_box_positions[k]) {
                    // Box swap conflict
                    size_t agent_j = findAgentResponsibleForBox(j, solutions[depth], previous_agent_positions, previous_box_positions);
                    size_t agent_k = findAgentResponsibleForBox(k, solutions[depth], previous_agent_positions, previous_box_positions);
                    if (agent_j != SIZE_MAX && agent_k != SIZE_MAX && agent_j != agent_k) {
                        return FullConflict(agent_j + FIRST_AGENT, agent_k + FIRST_AGENT, Constraint(current_box_positions[j], depth + 1));
                    }
                }
            }
        }

        // 5. Agent-Box Vertex conflicts
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            for (size_t k = 0; k < current_box_positions.size(); ++k) {
                if (current_agent_positions[j] == current_box_positions[k]) {
                    size_t agent_k = findAgentResponsibleForBox(k, solutions[depth], previous_agent_positions, previous_box_positions);
                    if (agent_k != SIZE_MAX && j != agent_k) {
                        return FullConflict(j + FIRST_AGENT, agent_k + FIRST_AGENT, Constraint(current_agent_positions[j], depth + 1));
                    }
                }
            }
        }

        // 6. Agent-Box Follow conflicts
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            for (size_t k = 0; k < current_box_positions.size(); ++k) {
                // Agent moving to where box was, box moving to where agent was
                if (previous_box_positions[k] == current_agent_positions[j] && previous_agent_positions[j] == current_box_positions[k]) {
                    size_t agent_k = findAgentResponsibleForBox(k, solutions[depth], previous_agent_positions, previous_box_positions);
                    if (agent_k != SIZE_MAX && j != agent_k) {
                        return FullConflict(j + FIRST_AGENT, agent_k + FIRST_AGENT, Constraint(current_agent_positions[j], depth + 1));
                    }
                }
            }
        }
    }

    return FullConflict(0, 0, Constraint(Cell2D(0, 0), 0));
}

std::pair<std::map<char, std::pair<uint_fast8_t, uint_fast8_t>>, size_t> CBS::buildAgentMapping(
    const std::vector<LowLevelState *> &agent_states) {
    std::map<char, std::pair<uint_fast8_t, uint_fast8_t>> mapping;
    size_t total_agents = 0;

    for (uint_fast8_t group_idx = 0; group_idx < agent_states.size(); group_idx++) {
        const auto &agents = agent_states[group_idx]->agents;
        for (uint_fast8_t agent_idx = 0; agent_idx < agents.size(); agent_idx++) {
            char symbol = agents[agent_idx].getSymbol();
            mapping[symbol] = {group_idx, agent_idx};
            total_agents++;
        }
    }

    return {std::move(mapping), total_agents};
}
