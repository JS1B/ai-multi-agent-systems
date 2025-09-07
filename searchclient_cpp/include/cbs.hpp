#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
// #include <vector>

#include "action.hpp"
#include "conflict.hpp"
#include "constraint.hpp"
#include "frontier.hpp"
#include "graphsearch.hpp"
#include "low_level_state.hpp"
#include "memory.hpp"
#include "utils.hpp"

// Global start time
static auto start_time = std::chrono::high_resolution_clock::now();

class CTNode {
   public:
    CTNode() : cost(0) {}
    ~CTNode() = default;

    std::vector<std::vector<std::vector<const Action *>>> solutions;
    // std::vector<OneSidedConflict> one_sided_conflicts;
    std::set<OneSidedConflict> one_sided_conflicts;
    size_t cost;

    struct NodeComparator {
        bool operator()(const CTNode *a, const CTNode *b) const {
            if (a->cost != b->cost) {
                return a->cost > b->cost;  // Lower cost has higher priority
            }
            // Tie-breaking: prefer nodes with fewer constraints
            return a->one_sided_conflicts.size() > b->one_sided_conflicts.size();
        }
    };

    // For duplicate detection
    bool operator==(const CTNode &other) const { return one_sided_conflicts == other.one_sided_conflicts; }

    struct NodeHash {
        std::size_t operator()(const CTNode *node) const {
            std::size_t hash = 0;
            for (const auto &conflict : node->one_sided_conflicts) {
                hash ^= std::hash<char>{}(conflict.a1_symbol) ^ std::hash<int>{}(conflict.constraint.vertex.r) ^
                        std::hash<int>{}(conflict.constraint.vertex.c) ^ std::hash<size_t>{}(conflict.constraint.g);
            }
            return hash;
        }
    };
};

class CBSFrontier {
   private:
    std::priority_queue<CTNode *, std::vector<CTNode *>, CTNode::NodeComparator> queue_;

   public:
    CBSFrontier() {}
    ~CBSFrontier() {
        while (!isEmpty()) {
            delete pop();
        }
        queue_ = std::priority_queue<CTNode *, std::vector<CTNode *>, CTNode::NodeComparator>();
    }
    void add(CTNode *node) { queue_.push(node); }

    CTNode *pop() {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty CBS frontier.");
        }
        CTNode *node = queue_.top();
        queue_.pop();
        return node;
    }
    bool isEmpty() const { return queue_.empty(); }
    size_t size() const { return queue_.size(); }
};

void printSearchStatus(const CBSFrontier &cbs_frontier, const size_t &generated_states_count);

class CBS {
   public:
    const Level &initial_level;

   private:
    std::vector<LowLevelState *> initial_agents_states_;
    size_t agents_num_;
    std::set<std::set<OneSidedConflict>> visited_constraint_sets_;

    // Fast lookup from agent symbol to (group_idx, agent_idx_within_group)
    std::map<char, std::pair<uint_fast8_t, uint_fast8_t>> agent_symbol_to_group_info_;
    size_t total_agents_;

   public:
    CBS() = delete;
    CBS(const Level &loaded_level);
    ~CBS();

    std::vector<std::vector<const Action *>> solve();

   private:
    std::vector<std::vector<const Action *>> mergePlans(std::vector<std::vector<std::vector<const Action *>>> &plans);

    FullConflict findFirstConflict(const std::vector<std::vector<const Action *>> &solutions) const;
    size_t findAgentResponsibleForBox(size_t box_index, const std::vector<const Action *> &actions,
                                      const std::vector<Cell2D> &previous_agent_positions,
                                      const std::vector<Cell2D> &previous_box_positions) const;

    // Helper function to build agent symbol mapping
    static std::pair<std::map<char, std::pair<uint_fast8_t, uint_fast8_t>>, size_t> buildAgentMapping(
        const std::vector<LowLevelState *> &agent_states);
};
