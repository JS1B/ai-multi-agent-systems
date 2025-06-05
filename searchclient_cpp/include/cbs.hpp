#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

#include "action.hpp"
#include "constraint.hpp"
#include "frontier.hpp"
#include "graphsearch.hpp"
#include "low_level_state.hpp"
#include "memory.hpp"
#include "utils.hpp"

// Global start time
static auto start_time = std::chrono::high_resolution_clock::now();

// void printSearchStatus(const std::unordered_set<State *, StatePtrHash, StatePtrEqual> &explored, const Frontier &frontier) {
//     static bool first_time = true;
//     if (first_time) {
//         first_time = false;
//         printf("#Expanded, Frontier, Generated, Time[s], Alloc[MB], MaxAlloc[MB]\n");
//     }

//     const auto now = std::chrono::high_resolution_clock::now();
//     const auto elapsed_time = std::chrono::duration<double>(now - start_time).count();

//     const size_t size_explored = explored.size();
//     const size_t size_frontier = frontier.size();

//     fprintf(stdout, "#%8zu, %8zu, %9zu, %7.3f, %9u, %12u\n", size_explored, size_frontier, size_explored + size_frontier, elapsed_time,
//             Memory::getUsage(), Memory::maxUsage);
//     fflush(stdout);
// }

// void printSearchStatus(const std::vector<AgentGraphSearch *> &agent_searches) {
//     for (auto agent_search : agent_searches) {
//         fprintf(stdout, "Agent %d: %d\n", agent_search->getAgentIdx(), agent_search->getSolutionLength());
//     }
//     fflush(stdout);
// }

class CTNode {
   public:
    CTNode() : cost(0) {}
    ~CTNode() = default;

    std::vector<std::vector<const Action *>> solutions;
    std::vector<Constraint> constraints;
    size_t cost;

    struct NodeComparator {
        bool operator()(const CTNode *a, const CTNode *b) const { return a->cost < b->cost; }
    };
};

class CBS {
   private:
    const StaticLevel static_level_;
    std::vector<LowLevelState *> agents_states_;
    std::vector<Graphsearch *> agent_searches_;
    size_t agents_num_;

   public:
    CBS(const Level &loaded_level) : static_level_(loaded_level.static_level) {
        for (const auto &agent_bulk : loaded_level.agent_bulks) {
            for (const auto &splitted_bulk : agent_bulk.split()) {
                agents_states_.push_back(new LowLevelState(splitted_bulk, static_level_));
            }
        }

        agents_num_ = agents_states_.size();

        // Create AgentGraphSearch objects for each agent_state
        agent_searches_.reserve(agents_states_.size());
        for (auto agent_state : agents_states_) {
            agent_searches_.push_back(new Graphsearch(agent_state, new FrontierBestFirst(new HeuristicAStar())));
        }
    }

    ~CBS() {
        for (auto agent_search : agent_searches_) {
            delete agent_search;
        }
    }

    std::vector<std::vector<const Action *>> solve() {
        int iterations = 0;
        CTNode root;
        // root.constraints.reserve(agents_states_.size());
        // root.solutions.reserve(agents_states_.size());

        std::vector<std::vector<std::vector<const Action *>>> solutions_per_search(agent_searches_.size());
        // Find a solution for each agent bulk
        for (size_t i = 0; i < agent_searches_.size(); i++) {
            auto bulk_plan = agent_searches_[i]->solve();
            if (bulk_plan.empty()) {
                return {};
            }
            solutions_per_search[i] = bulk_plan;
        }

        // Make all plans the same length
        size_t longest_plan_length = 0;
        for (size_t i = 0; i < solutions_per_search.size(); i++) {
            longest_plan_length = std::max(longest_plan_length, solutions_per_search[i].size());
        }

        for (size_t i = 0; i < solutions_per_search.size(); i++) {
            size_t agents_in_bulk = solutions_per_search[i][0].size();
            solutions_per_search[i].resize(longest_plan_length, std::vector<const Action *>(agents_in_bulk, (const Action *)&Action::NoOp));
        }

        // Flatten
        std::vector<const Action *> row;
        row.reserve(agents_num_);

        for (size_t depth = 0; depth < longest_plan_length; depth++) {
            for (size_t i = 0; i < solutions_per_search.size(); i++) {
                row.insert(row.end(), solutions_per_search[i][depth].begin(), solutions_per_search[i][depth].end());
            }
            root.solutions.push_back(row);
            row.clear();
        }

        root.cost = utils::SIC(root.solutions);

        std::priority_queue<CTNode *, std::vector<CTNode *>, CTNode::NodeComparator> cbs_frontier;
        cbs_frontier.push(&root);

        while (!cbs_frontier.empty()) {
            iterations++;

            CTNode *node = cbs_frontier.top();
            cbs_frontier.pop();

            if (isValid(node->solutions)) {
                return node->solutions;
            }

            Constraint conflict = findFirstConflict(node->solutions);

            // if (iterations % 100000 == 1) {
            //     printSearchStatus(agent_searches_);
            // }
        }

        return {};
    }

   private:
    bool isValid(const std::vector<std::vector<const Action *>> &solutions) const {
        // TODO: check for conflicts
        return true;
    }

    Constraint findFirstConflict(const std::vector<std::vector<const Action *>> &solutions) const {
        // TODO: find first conflict
        return Constraint(Cell2D(0, 0), 0);
    }
};
