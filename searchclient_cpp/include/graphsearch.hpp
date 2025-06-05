#pragma once

#include <string>
#include <vector>

#include "constraint.hpp"
#include "frontier.hpp"
#include "low_level_state.hpp"
#include "memory.hpp"
#include "state.hpp"

class Graphsearch {
   private:
    LowLevelState *initial_state_;
    Frontier *frontier_;
    std::unordered_set<LowLevelState *, LowLevelStatePtrHash, LowLevelStatePtrEqual> explored_;
    std::vector<const Constraint *> constraints_;

   public:
    Graphsearch() = delete;
    Graphsearch(LowLevelState *initial_state, Frontier *frontier) : initial_state_(initial_state), frontier_(frontier) {}
    Graphsearch(const Graphsearch &) = delete;
    Graphsearch &operator=(const Graphsearch &) = delete;
    ~Graphsearch() = default;

    std::vector<std::vector<const Action *>> solve() {
        int iterations = 0;
        frontier_->add(initial_state_);
        explored_.reserve(10'000);

        while (true) {
            if (iterations % 1000 == 0) {
                if (Memory::getUsage() > Memory::maxUsage) {
                    fprintf(stderr, "Maximum memory usage exceeded.\n");
                    return {};
                }
            }

            if (frontier_->isEmpty()) {
                fprintf(stderr, "Frontier is empty.\n");
                return {};
            }

            LowLevelState *state = frontier_->pop();

            if (state->isGoalState()) {
                return state->extractPlan();
            }

            explored_.insert(state);

            auto expanded_states = state->getExpandedStates();
            for (auto child : expanded_states) {
                if (explored_.find(child) == explored_.end() && !frontier_->contains(child)) {
                    frontier_->add(child);
                    continue;
                }
                delete child;
            }
        }
    }
};

// struct AgentNode {
//     Cell2D agent_pos;
//     CharGrid current_boxes_config;  // Boxes as modified by *this agent's* plan so far
//     int time_offset;                // Time elapsed since this low-level plan started (0, 1, 2...)

//     int g_cost;  // Actual cost (time steps) from this plan's start
//     int h_cost;  // Heuristic to this agent's specific goal

//     // For path reconstruction
//     const LowLevelPlannerNode *parent;  // Pointer to previous node in this A* search
//     const Action *action_taken;         // Action that led to this node

//     // Details if the action_taken moved a box
//     bool box_was_moved_by_action = false;
//     char moved_box_id = 0;
//     Cell2D moved_box_original_pos;
//     Cell2D moved_box_new_pos;

//     LowLevelPlannerNode(Cell2D ap, const CharGrid &bc, int to, int g, int h, const LowLevelPlannerNode *p, const Action *act)
//         : agent_pos(ap), current_boxes_config(bc), time_offset(to), g_cost(g), h_cost(h), parent(p), action_taken(act) {}

//     int f_cost() const { return g_cost + h_cost; }

//     // For priority queue (min-heap)
//     bool operator>(const LowLevelPlannerNode &other) const {
//         if (f_cost() == other.f_cost()) {
//             if (h_cost == other.h_cost) {
//                 return g_cost > other.g_cost;  // Tie-break on g_cost
//             }
//             return h_cost > other.h_cost;  // Standard tie-breaking
//         }
//         return f_cost() > other.f_cost();
//     }

//     // Equality for closed set: agent_pos, time_offset, and box_config
//     bool operator==(const LowLevelPlannerNode &other) const {
//         return agent_pos == other.agent_pos && time_offset == other.time_offset && current_boxes_config == other.current_boxes_config;
//     }
// };

// // Custom hash for LowLevelPlannerNode for unordered_set
// namespace std {
// template <>
// struct hash<LowLevelPlannerNode> {
//     size_t operator()(const LowLevelPlannerNode &node) const {
//         size_t h1 = std::hash<int_fast8_t>()(node.agent_pos.r);
//         size_t h2 = std::hash<int_fast8_t>()(node.agent_pos.c);
//         size_t h3 = std::hash<int>()(node.time_offset);
//         size_t h4 = node.current_boxes_config.get_hash();
//         // Combine hashes (from utils::hashCombine or similar)
//         size_t seed = 0;
//         seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//         seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//         seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//         seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//         return seed;
//     }
// };
// }  // namespace std