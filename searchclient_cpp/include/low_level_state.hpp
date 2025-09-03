#pragma once

#include <cassert>
#include <vector>

#include "action.hpp"
#include "agent.hpp"
#include "level.hpp"

class LowLevelState {
   private:
    const size_t g_;
    const StaticLevel &static_level_;
    mutable size_t hash_ = 0;

   public:
    LowLevelState() = delete;
    LowLevelState(std::vector<Agent> agents, const StaticLevel &static_level)
        : g_(0), static_level_(static_level), agents(agents), parent(nullptr) {
        agents.shrink_to_fit();
    }

    LowLevelState(const LowLevelState &other)
        : g_(other.g_), static_level_(other.static_level_), agents(other.agents), parent(other.parent), actions(other.actions) {}

    LowLevelState *clone() const { return new LowLevelState(*this); }

    std::vector<Agent> agents;
    const LowLevelState *parent;
    std::vector<const Action *> actions;

    inline size_t getG() const { return g_; }

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

    std::vector<LowLevelState *> getExpandedStates() const {
        const auto &actions_permutations = Action::getAllPermutations(agents.size());
        std::vector<LowLevelState *> expanded_states;
        expanded_states.reserve(actions_permutations.size() * agents.size());

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
        for (const auto &agent : agents) {
            hash_ = hash_ * 31 + agent.getHash();
        }
        hash_ = hash_ * 31 + g_;
        return hash_;
    }

    bool operator==(const LowLevelState &other) const { return agents == other.agents && g_ == other.g_; }
    bool isGoalState() const {
        return std::all_of(agents.begin(), agents.end(), [](const Agent &agent) { return agent.reachedGoal(); });
    }

    bool isApplicable(const std::vector<const Action *> &joint_actions) const {
        assert(joint_actions.size() == agents.size());

        bool is_applicable = false;
        for (size_t i = 0; i < joint_actions.size(); i++) {
            const Action *action = joint_actions[i];
            Cell2D agent_pos = agents[i].getPosition();

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
                    Cell2D current_box_position = agent_pos + action->agent_delta;

                    // char box_id = static_level_.all_boxes(current_box_position);
                    // if (!box_id) {
                    //     return false;
                    // }

                    // Cell2D destination = box_position + action->box_delta;
                    // if (!static_level_.isCellFree(destination)) {
                    //     return false;
                    // }
                    // return static_level_.agent_colors[static_level_.agent_idx] == static_level_.box_colors[box_id - FIRST_BOX];
                    is_applicable = false;
                    break;
                }

                case ActionType::Pull: {
                    // Box will be in a moment where the agent is now
                    // Cell2D box_position = agent_pos - action->box_delta;
                    // char box_id = static_level_.all_boxes(box_position);
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

    void applyActions(const std::vector<const Action *> &joint_actions) {
        assert(joint_actions.size() == agents.size());

        for (size_t i = 0; i < joint_actions.size(); i++) {
            const Action *action = joint_actions[i];
            Cell2D &agent_pos_ref = agents[i].position();

            switch (action->type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    agent_pos_ref += action->agent_delta;
                    break;

                case ActionType::Push: {
                    Cell2D box_pos = agent_pos_ref + action->agent_delta;
                    // agent_pos_ref += action->agent_delta;
                    // agent_bulk.all_boxes(box_pos + action->box_delta) = agent_bulk.all_boxes(box_pos);
                    // agent_bulk.all_boxes(box_pos) = 0;
                    break;
                }

                case ActionType::Pull: {
                    Cell2D box_pos = agent_pos_ref - action->box_delta;
                    // agent_pos_ref += action->agent_delta;
                    // agent_bulk.all_boxes(box_pos + action->box_delta) = agent_bulk.all_boxes(box_pos);
                    // agent_bulk.all_boxes(box_pos) = 0;
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

   private:
    LowLevelState(const LowLevelState *parent, const std::vector<const Action *> joint_actions)
        : g_(parent->g_ + 1), static_level_(parent->static_level_), agents(parent->agents), parent(parent), actions(joint_actions) {
        applyActions(joint_actions);
    }

    bool isCellFree(const Cell2D &cell) const {
        if (!static_level_.isCellFree(cell)) return false;

        for (size_t i = 0; i < agents.size(); i++) {
            if (agents[i].getPosition() == cell) {
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