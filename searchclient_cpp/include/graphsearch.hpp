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
    size_t generated_states_count_;
    bool solution_found_;

   public:
    Graphsearch() = delete;
    Graphsearch(LowLevelState *initial_state, Frontier *frontier)
        : initial_state_(initial_state), frontier_(frontier), generated_states_count_(0), solution_found_(false) {
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

    bool isTemporallyExplored(const LowLevelState *state, const std::vector<Constraint> &constraints) const {
        // If no constraints at all, use standard duplicate detection
        if (constraints.empty()) {
            for (const auto *explored_state : explored_) {
                if (*state == *explored_state) {
                    return true;
                }
            }
            return false;
        }

        // Check if any state in explored set is temporally equivalent to this state
        // Optimize by limiting search scope to prevent performance degradation
        // size_t checks = 0;
        for (const auto *explored_state : explored_) {
            // if (++checks > 500) break;  // Cap expensive searches
            if (state->temporalEquals(*explored_state, constraints)) {
                return true;
            }
        }
        return false;
    }

    // void printSearchStatus() const {
    //     fprintf(stdout, "Agent %d: %d\n", initial_state_->agent_bul.getColor(), initial_state_->agent_bulk.getSymbol());
    //     fflush(stdout);
    // }

    std::vector<std::vector<const Action *>> solve(const std::vector<Constraint> &constraints) {
        size_t iterations = 0;

        // Reset tracking variables for new search
        generated_states_count_ = 0;
        solution_found_ = false;

        // Clear frontier and explored set for new search
        frontier_->clear();
        explored_.clear();

        frontier_->add(initial_state_->clone());

        while (true) {
            if (iterations % 100 == 0 && Memory::getUsage() > Memory::maxUsage) {
                fprintf(stderr, "Maximum memory usage exceeded low level search.\n");
                return {};
            }

            if (frontier_->isEmpty()) {
                // fprintf(stderr, "Frontier is empty.\n");
                return {};
            }

            LowLevelState *state = frontier_->pop();

            if (state->isGoalState() && areConstraintsSatisfied(state, constraints)) {
                solution_found_ = true;
                return state->extractPlan();
            }

            auto expanded_states = state->getExpandedStates();
            generated_states_count_ += expanded_states.size();

            for (auto child : expanded_states) {
                bool explored = isTemporallyExplored(child, constraints);
                bool in_frontier = frontier_->contains(child);
                bool constraints_satisfied = areConstraintsSatisfied(child, constraints);
                if (!explored && !in_frontier && constraints_satisfied) {
                    frontier_->add(child);
                    continue;
                }
                delete child;
            }

            explored_.insert(state);
            iterations++;
        }
    }

    // Getter methods for tracking information
    size_t getGeneratedStatesCount() const { return generated_states_count_; }
    bool wasSolutionFound() const { return solution_found_; }
};
