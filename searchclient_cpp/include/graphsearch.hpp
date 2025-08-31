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

   public:
    Graphsearch() = delete;
    Graphsearch(LowLevelState *initial_state, Frontier *frontier) : initial_state_(initial_state), frontier_(frontier) {
        explored_.reserve(1'000);
    }
    Graphsearch(const Graphsearch &) = delete;
    Graphsearch &operator=(const Graphsearch &) = delete;
    ~Graphsearch() { delete frontier_; }

    bool areConstraintsSatisfied(const LowLevelState *state, const std::vector<Constraint> &constraints) const {
        for (const auto &constraint : constraints) {
            if (constraint.g != state->getG()) {
                continue;
            }

            for (size_t i = 0; i < state->agents.size(); ++i) {
                if (constraint.vertex == state->agents[i].getPosition()) {
                    return false;
                }
            }
        }
        return true;
    }

    // void printSearchStatus() const {
    //     fprintf(stdout, "Agent %d: %d\n", initial_state_->agent_bul.getColor(), initial_state_->agent_bulk.getSymbol());
    //     fflush(stdout);
    // }

    std::vector<std::vector<const Action *>> solve(const std::vector<Constraint> &constraints) {
        int iterations = 0;

        // Clear frontier and explored set for new search
        frontier_->clear();
        explored_.clear();

        frontier_->add(initial_state_);

        while (true) {
            if (iterations % 1000 == 0 && Memory::getUsage() > Memory::maxUsage) {
                fprintf(stderr, "Maximum memory usage exceeded low level search.\n");
                return {};
            }

            if (frontier_->isEmpty()) {
                // fprintf(stderr, "Frontier is empty.\n");
                return {};
            }

            LowLevelState *state = frontier_->pop();

            if (state->isGoalState() && areConstraintsSatisfied(state, constraints)) {
                return state->extractPlan();
            }

            explored_.insert(state);

            auto expanded_states = state->getExpandedStates();
            for (auto child : expanded_states) {
                bool explored = explored_.find(child) != explored_.end();
                bool in_frontier = frontier_->contains(child);
                bool constraints_satisfied = areConstraintsSatisfied(child, constraints);
                if (!explored && !in_frontier && constraints_satisfied) {
                    frontier_->add(child);
                    continue;
                }
                delete child;
            }

            iterations++;
        }
    }
};
