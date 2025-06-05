#pragma once

#include <cassert>
#include <vector>

#include "action.hpp"
#include "entity_bulk.hpp"
#include "level.hpp"

class LowLevelState {
   private:
    const int g_;
    const StaticLevel &static_level_;
    mutable size_t hash_ = 0;

   public:
    LowLevelState() = delete;
    LowLevelState(EntityBulk agent_bulk, const StaticLevel &static_level)
        : g_(0), static_level_(static_level), agent_bulk(agent_bulk), parent(nullptr) {}

    EntityBulk agent_bulk;
    const LowLevelState *parent;
    std::vector<const Action *> actions;

    inline int getG() const { return g_; }

    std::vector<std::vector<const Action *>> extractPlan() const {
        std::vector<std::vector<const Action *>> plan;
        plan.reserve(g_);
        const LowLevelState *current = this;

        while (current->parent != nullptr) {
            plan.push_back(current->actions);
            current = current->parent;
        }
        std::reverse(plan.begin(), plan.end());
        return plan;
    }

    // std::vector<const AgentState *> extractStates() const {
    //     std::vector<const AgentState *> states;
    //     states.reserve(g_);
    //     const AgentState *current = this;
    //     while (current != nullptr) {
    //         states.push_back(current);
    //         current = current->parent;
    //     }
    //     // std::reverse(states.begin(), states.end());
    //     return states;
    // }

    std::vector<LowLevelState *> getExpandedStates() const {
        auto actions_permutations = Action::getAllPermutations(agent_bulk.size());
        std::vector<LowLevelState *> expanded_states;
        expanded_states.reserve(actions_permutations.size() * agent_bulk.size());

        for (const auto &actions_permutation : actions_permutations) {
            if (!isApplicable(actions_permutation)) {
                continue;
            }
            expanded_states.push_back(new LowLevelState(this, actions_permutation));
        }
        return expanded_states;
    }

    size_t getHash() const {
        if (hash_ != 0) {
            return hash_;
        }
        hash_ = agent_bulk.getHash();
        return hash_;
    }

    bool operator==(const LowLevelState &other) const { return agent_bulk == other.agent_bulk; }
    bool isGoalState() const { return agent_bulk.reachedGoal(); }

    bool isApplicable(const std::vector<const Action *> &joint_actions) const {
        assert(joint_actions.size() == agent_bulk.size());

        bool is_applicable = false;
        for (size_t i = 0; i < joint_actions.size(); i++) {
            const Action *action = joint_actions[i];
            Cell2D agent_pos = agent_bulk.getPosition(i);

            switch (action->type) {
                case ActionType::NoOp:
                    is_applicable = true;
                    break;

                case ActionType::Move: {
                    Cell2D destination = agent_pos + action->agent_delta;
                    is_applicable = isCellFree(destination);
                    break;
                }

                case ActionType::Push: {
                    Cell2D box_position = agent_pos + action->agent_delta;
                    // char box_id = level.all_boxes(box_position);
                    // if (!box_id) {
                    //     return false;
                    // }

                    // Cell2D destination = box_position + action->box_delta;
                    // if (!level.isCellFree(destination)) {
                    //     return false;
                    // }
                    // return level.agent_colors[level.agent_idx] == level.box_colors[box_id - FIRST_BOX];
                    is_applicable = false;
                    break;
                }

                case ActionType::Pull: {
                    // Box will be in a moment where the agent is now
                    Cell2D box_position = agent_pos - action->box_delta;
                    // char box_id = level.all_boxes(box_position);
                    // if (!box_id) {
                    //     return false;
                    // }

                    // Cell2D destination = level.agent_pos + action.agent_delta;
                    // if (!level.isCellFree(destination)) {
                    //     return false;
                    // }
                    // return level.agent_colors[level.agent_idx] == level.box_colors[box_id - FIRST_BOX];
                    is_applicable = false;
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
        return is_applicable;
    }

   private:
    LowLevelState(const LowLevelState *parent, const std::vector<const Action *> actions)
        : g_(parent->g_ + 1), static_level_(parent->static_level_), agent_bulk(parent->agent_bulk), parent(parent), actions(actions) {
        assert(actions.size() == agent_bulk.size());

        for (size_t i = 0; i < actions.size(); i++) {
            const Action *action = actions[i];
            Cell2D &agent_pos_ref = agent_bulk.position(i);

            switch (action->type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    agent_pos_ref += action->agent_delta;
                    break;

                case ActionType::Push: {
                    Cell2D box_pos = agent_pos_ref + action->agent_delta;
                    agent_pos_ref += action->agent_delta;
                    // agent_bulk.all_boxes(box_pos + action->box_delta) = agent_bulk.all_boxes(box_pos);
                    // agent_bulk.all_boxes(box_pos) = 0;
                    break;
                }

                case ActionType::Pull: {
                    Cell2D box_pos = agent_pos_ref - action->box_delta;
                    agent_pos_ref += action->agent_delta;
                    // agent_bulk.all_boxes(box_pos + action->box_delta) = agent_bulk.all_boxes(box_pos);
                    // agent_bulk.all_boxes(box_pos) = 0;
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

    bool isCellFree(const Cell2D &cell) const {
        if (!static_level_.isCellFree(cell)) return false;

        for (size_t i = 0; i < agent_bulk.size(); i++) {
            if (agent_bulk.getPosition(i) == cell) {
                return false;
            }
        }
        return true;
    }
};

struct LowLevelStatePtrHash {
    std::size_t operator()(const LowLevelState *state_ptr) const {
        if (!state_ptr) {
            return 0;
        }
        return state_ptr->getHash();
    }
};

struct LowLevelStatePtrEqual {
    bool operator()(const LowLevelState *lhs_ptr, const LowLevelState *rhs_ptr) const {
        if (lhs_ptr == rhs_ptr) {
            return true;
        }
        if (!lhs_ptr || !rhs_ptr) {
            return false;
        }
        return *lhs_ptr == *rhs_ptr;
    }
};