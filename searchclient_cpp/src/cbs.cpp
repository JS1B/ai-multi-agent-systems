#include "cbs.hpp"

// C++
#include <cassert>
#include <queue>
#include <unordered_map>

#include "memory.hpp"

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        // Simple hash combining. boost::hash_combine is more robust.
        return h1 ^ (h2 << 1);
    }
};

void printSearchStatus(const CBSFrontier &cbs_frontier, const size_t &generated_states_count) {
    static bool first_time = true;

    if (first_time) {
        fprintf(stdout, "#CBS fr size, Mem usage[MB], Generated states\n");
        first_time = false;
    }

    fprintf(stdout, "#%11zu, %13d, %13zu\n", cbs_frontier.size(), Memory::getUsage(), generated_states_count);
    fflush(stdout);
}

CBS::CBS(const Level &loaded_level) : initial_level_(loaded_level) {
    for (const auto &agent : loaded_level.agents) {
        initial_agents_states_.push_back(new LowLevelState(initial_level_.static_level, {agent}, loaded_level.boxes));
    }

    agents_num_ = initial_agents_states_.size();
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
            size_t agent_idx = agent_symbol - FIRST_AGENT;

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

            Graphsearch *agent_search = agent_searches[agent_idx];
            auto plan = agent_search->solve(constraints);
            generated_states_count += agent_search->getGeneratedStatesCount();
            child->solutions[agent_idx] = plan;
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

    // Make all plans the same length
    size_t longest_plan_length = 0;
    for (size_t i = 0; i < plans_copy.size(); i++) {
        longest_plan_length = std::max(longest_plan_length, plans_copy[i].size());
    }

    // Merge
    for (size_t i = 0; i < plans_copy.size(); i++) {
        plans_copy[i].resize(longest_plan_length, std::vector<const Action *>(plans_copy[i][0].size(), (const Action *)&Action::NoOp));
    }

    std::vector<const Action *> row;
    row.reserve(agents_num_);
    std::vector<std::vector<const Action *>> merged_plans;
    merged_plans.reserve(longest_plan_length);

    for (size_t depth = 0; depth < longest_plan_length; depth++) {
        for (size_t i = 0; i < plans_copy.size(); i++) {
            row.insert(row.end(), plans_copy[i][depth].begin(), plans_copy[i][depth].end());
        }
        merged_plans.push_back(row);
        row.clear();
    }

    return merged_plans;
}

FullConflict CBS::findFirstConflict(const std::vector<std::vector<const Action *>> &solutions) const {
    assert(solutions[0].size() == initial_level_.agents.size());

    std::vector<Cell2D> current_agent_positions(solutions[0].size());
    std::vector<Cell2D> previous_agent_positions(solutions[0].size());

    // Initialize positions
    for (size_t i = 0; i < solutions[0].size(); i++) {
        current_agent_positions[i] = initial_level_.agents[i].getPosition();
        previous_agent_positions[i] = initial_level_.agents[i].getPosition();
    }

    for (size_t depth = 0; depth < solutions.size(); depth++) {
        // Update position history
        previous_agent_positions = current_agent_positions;

        // Apply actions
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            current_agent_positions[j] = previous_agent_positions[j] + solutions[depth][j]->agent_delta;
        }

        // Vertex conflicts
        for (size_t j = 0; j < solutions[0].size(); ++j) {
            for (size_t k = j + 1; k < solutions[0].size(); ++k) {
                if (current_agent_positions[j] == current_agent_positions[k]) {
                    return FullConflict(j + FIRST_AGENT, k + FIRST_AGENT, Constraint(current_agent_positions[j], depth + 1));
                }
            }
        }

        // Follow/trailing conflicts
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
    }

    return FullConflict(0, 0, Constraint(Cell2D(0, 0), 0));
}
