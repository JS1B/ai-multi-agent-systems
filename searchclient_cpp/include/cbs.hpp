#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

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
   private:
    mutable size_t hash_ = 0;

   public:
    CTNode();
    ~CTNode() = default;

    std::vector<std::vector<std::vector<const Action *>>> solutions;
    std::vector<Conflict> conflicts;
    size_t cost;

    size_t getHash() const {
        if (hash_ != 0) {
            return hash_;
        }
        size_t current_hash = 0;
        // Use a robust hash combination function (e.g., from boost::hash_combine idea)
        // For simplicity here, will use `^ (val << 1)`.
        // Example: Hashing vector of vectors of Action pointers (deep hash needed)
        for (const auto &solution_bulk : solutions) {  // solutions is vector<vector<vector<const Action*>>>
            // For each bulk_plan (vector<vector<const Action*>>)
            current_hash ^= std::hash<size_t>()(solution_bulk.size()) + 0x9e3779b9 + (current_hash << 6) + (current_hash >> 2);
            for (const auto &plan_per_agent : solution_bulk) {  // plan_per_agent (vector<const Action*>)
                current_hash ^= std::hash<size_t>()(plan_per_agent.size()) + 0x9e3779b9 + (current_hash << 6) + (current_hash >> 2);
                for (const Action *action : plan_per_agent) {
                    // Deep hash the action, assuming Action has a meaningful hashable state
                    // This might require a custom hash for `const Action *` based on its data.
                    current_hash ^= std::hash<const Action *>()(action) + 0x9e3779b9 + (current_hash << 6) + (current_hash >> 2);
                }
            }
        }
        // Deep hash the constraints
        for (const auto &conflict : conflicts) {
            // Assuming Constraint has an appropriate hash method or can be hashed via its members
            current_hash ^= std::hash<Cell2D>()(conflict.constraint.vertex) + std::hash<size_t>()(conflict.constraint.g) + 0x9e3779b9 +
                            (current_hash << 6) + (current_hash >> 2);
            // Add constraint.type as well if it's relevant to equality
        }
        hash_ = current_hash;  // Store the computed hash
        return hash_;
    }

    bool operator==(const CTNode &other) const {
        if (cost != other.cost) {
            return false;
        }
        return solutions == other.solutions && conflicts == other.conflicts;
    }

    struct NodeComparator {
        bool operator()(const CTNode *a, const CTNode *b) const { return a->cost > b->cost; }
    };
};

struct CTNodePtrHash {
    size_t operator()(const CTNode *node) const { return node->getHash(); }
};

struct CTNodePtrEqual {
    bool operator()(const CTNode *a, const CTNode *b) const {
        if (a == b) {
            return true;
        }
        if (!a || !b) {
            return false;
        }
        return *a == *b;
    }
};

class CBSFrontier {
   private:
    std::priority_queue<CTNode *, std::vector<CTNode *>, CTNode::NodeComparator> queue_;
    std::unordered_set<CTNode *, CTNodePtrHash, CTNodePtrEqual> set_;
    std::unordered_set<CTNode *, CTNodePtrHash, CTNodePtrEqual> closeset_;

   public:
    CBSFrontier() { set_.reserve(1'000); }
    ~CBSFrontier() {
        for (auto node : set_) {
            delete node;
        }
    }
    void add(CTNode *node) {
        queue_.push(node);
        set_.insert(node);
    }
    CTNode *pop() {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty CBS frontier.");
        }
        CTNode *node = queue_.top();
        queue_.pop();
        set_.erase(node);
        return node;
    }
    bool isEmpty() const { return queue_.empty(); }
    size_t size() const { return queue_.size(); }
    bool contains(CTNode *node) const { return set_.count(node); }
    std::string getName() const { return "CBS frontier"; }
};

void printSearchStatus(CBSFrontier &cbs_frontier);

class CBS {
   private:
    const Level initial_level_;
    std::vector<LowLevelState *> initial_agents_states_;
    size_t agents_num_;

   public:
    CBS() = delete;
    CBS(const Level &loaded_level);
    ~CBS();

    std::vector<std::vector<const Action *>> solve();

   private:
    std::vector<std::vector<const Action *>> mergePlans(std::vector<std::vector<std::vector<const Action *>>> &plans);

    Conflict findFirstConflict(const std::vector<std::vector<const Action *>> &solutions) const;
};
