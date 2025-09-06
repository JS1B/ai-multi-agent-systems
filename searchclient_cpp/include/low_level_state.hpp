#pragma once

#include <cassert>
#include <vector>

#include "action.hpp"
#include "agent.hpp"
#include "box_bulk.hpp"
#include "constraint.hpp"
#include "level.hpp"

class LowLevelState {
   private:
    const size_t g_;
    const StaticLevel &static_level_;
    mutable size_t hash_ = 0;

   public:
    LowLevelState() = delete;
    LowLevelState(const StaticLevel &static_level, const std::vector<Agent> &agents, const std::vector<BoxBulk> &boxes)
        : g_(0), static_level_(static_level), agents(agents), box_bulks(boxes), parent(nullptr) {
        this->agents.shrink_to_fit();
        box_bulks.shrink_to_fit();
    }

    LowLevelState(const LowLevelState &other)
        : g_(other.g_),
          static_level_(other.static_level_),
          agents(other.agents),
          box_bulks(other.box_bulks),
          parent(other.parent),
          actions(other.actions) {}

    LowLevelState *clone() const { return new LowLevelState(*this); }

    std::vector<Agent> agents;
    std::vector<BoxBulk> box_bulks;
    const LowLevelState *parent;
    std::vector<const Action *> actions;

    inline size_t getG() const { return g_; }

    // Box management methods
    const std::vector<BoxBulk> &getBoxBulks() const { return box_bulks; }
    std::vector<BoxBulk> &getBoxBulks() { return box_bulks; }
    const StaticLevel &getStaticLevel() const { return static_level_; }

    char getBoxAt(const Cell2D &position) const {
        for (const auto &bulk : box_bulks) {
            for (size_t i = 0; i < bulk.size(); i++) {
                if (bulk.getPosition(i) == position) {
                    return bulk.getSymbol();
                }
            }
        }
        return 0;  // No box
    }

    bool moveBox(const Cell2D &from, const Cell2D &to) {
        for (auto &bulk : box_bulks) {
            for (size_t i = 0; i < bulk.size(); i++) {
                if (bulk.getPosition(i) == from) {
                    bulk.position(i) = to;
                    return true;
                }
            }
        }
        return false;
    }

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
        for (const auto &bulk : box_bulks) {
            hash_ = hash_ * 31 + bulk.getHash();
        }
        return hash_;
    }

    bool operator==(const LowLevelState &other) const { return agents == other.agents && box_bulks == other.box_bulks; }

    // Special equality method for constraint-aware comparison
    bool temporalEquals(const LowLevelState &other, const std::vector<Constraint> &constraints) const {
        if (agents != other.agents || box_bulks != other.box_bulks) {
            return false;
        }

        // Check if either state has constraints at current timestep
        bool this_has_constraints = false;
        bool other_has_constraints = false;

        for (const auto &constraint : constraints) {
            if (constraint.g == g_) {
                this_has_constraints = true;
            }
            if (constraint.g == other.g_) {
                other_has_constraints = true;
            }
        }

        // If either has temporal constraints, they must be at same timestep
        if (this_has_constraints || other_has_constraints) {
            return g_ == other.g_;
        }

        // Even without constraints, we should prefer earlier states (shorter paths)
        // Only consider equal if at same depth to maintain CBS correctness
        return g_ == other.g_;
    }

    bool isGoalState() const {
        // Check if all agents reached their goals
        bool agents_at_goal = std::all_of(agents.begin(), agents.end(), [](const Agent &agent) { return agent.reachedGoal(); });

        // Check if all boxes reached their goals
        bool boxes_at_goal = std::all_of(box_bulks.begin(), box_bulks.end(), [](const BoxBulk &bulk) { return bulk.reachedGoal(); });

        return agents_at_goal && boxes_at_goal;
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
                    Cell2D box_position = agent_pos + action->agent_delta;
                    Cell2D box_destination = box_position + action->box_delta;

                    // Check if there's a box at the expected position
                    char box_id = getBoxAt(box_position);
                    if (!box_id) {
                        is_applicable = false;
                        break;
                    }

                    // Check if box destination is free
                    if (!isCellFree(box_destination)) {
                        is_applicable = false;
                        break;
                    }

                    // Check color compatibility (agent can only push boxes of same color)
                    Color agent_color = static_level_.getAgentColor(agents[i].getSymbol());
                    bool can_push = false;
                    for (const auto &bulk : box_bulks) {
                        if (bulk.getSymbol() == box_id && bulk.getColor() == agent_color) {
                            can_push = true;
                            break;
                        }
                    }
                    is_applicable = can_push;
                    break;
                }

                case ActionType::Pull: {
                    Cell2D box_position = agent_pos - action->box_delta;
                    Cell2D agent_destination = agent_pos + action->agent_delta;
                    // Box moves to agent's current position

                    // Check if there's a box at the expected position
                    char box_id = getBoxAt(box_position);
                    if (!box_id) {
                        is_applicable = false;
                        break;
                    }

                    // Check if agent destination is free
                    if (!isCellFree(agent_destination)) {
                        is_applicable = false;
                        break;
                    }

                    // Box destination (agent's current position) will be free because agent is moving away

                    // Check color compatibility
                    Color agent_color = static_level_.getAgentColor(agents[i].getSymbol());
                    bool can_pull = false;
                    for (const auto &bulk : box_bulks) {
                        if (bulk.getSymbol() == box_id && bulk.getColor() == agent_color) {
                            can_pull = true;
                            break;
                        }
                    }
                    is_applicable = can_pull;
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
                    Cell2D new_box_pos = box_pos + action->box_delta;
                    agent_pos_ref += action->agent_delta;
                    moveBox(box_pos, new_box_pos);
                    break;
                }

                case ActionType::Pull: {
                    Cell2D box_pos = agent_pos_ref - action->box_delta;
                    Cell2D new_box_pos = agent_pos_ref;  // Box moves to agent's current position
                    agent_pos_ref += action->agent_delta;
                    moveBox(box_pos, new_box_pos);
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

   private:
    LowLevelState(const LowLevelState *parent, const std::vector<const Action *> &joint_actions)
        : g_(parent->g_ + 1),
          static_level_(parent->static_level_),
          agents(parent->agents),
          box_bulks(parent->box_bulks),
          parent(parent),
          actions(joint_actions) {
        applyActions(joint_actions);
    }

    bool isCellFree(const Cell2D &cell) const {
        if (!static_level_.isCellFree(cell)) return false;

        for (size_t i = 0; i < agents.size(); i++) {
            if (agents[i].getPosition() == cell) {
                return false;
            }
        }

        // Check if there's a box at this cell
        if (getBoxAt(cell) != 0) {
            return false;
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