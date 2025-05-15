#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "action.hpp"
#include "frontier.hpp"
#include "memory.hpp"
#include "state.hpp"

// Global start time
static auto start_time = std::chrono::high_resolution_clock::now();

void printSearchStatus(const std::unordered_set<State *, StatePtrHash, StatePtrEqual> &explored, const Frontier &frontier) {
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        printf("#Expanded, Frontier, Generated, Time[s], Alloc[MB], MaxAlloc[MB]\n");
    }

    const auto now = std::chrono::high_resolution_clock::now();
    const auto elapsed_time = std::chrono::duration<double>(now - start_time).count();

    const size_t size_explored = explored.size();
    const size_t size_frontier = frontier.size();

    printf("#%8zu, %8zu, %9zu, %7.3f, %9u, %12u\n", size_explored, size_frontier, size_explored + size_frontier, elapsed_time,
           Memory::getUsage(), Memory::maxUsage);
}

std::vector<std::vector<const Action *>> search(State *initial_state_param, Frontier *frontier) {
    int iterations = 0;

    std::unordered_set<State *, StatePtrHash, StatePtrEqual> explored;
    explored.reserve(400000);

    auto cleanupStates = [&](State* goal_state_to_preserve = nullptr) {
        std::vector<State*> frontier_states_to_delete;
        while (!frontier->isEmpty()) {
            State* s = frontier->pop(); // Pop first, comparisons happen here
            // Check for preservation *before* adding to delete list
            if (s != goal_state_to_preserve && s != initial_state_param) {
                frontier_states_to_delete.push_back(s);
            } else if (s == goal_state_to_preserve || s == initial_state_param) {
                // If it's a preserved state popped from frontier, it shouldn't be deleted later by explored loop either.
                // This case is tricky if it was also in explored. For now, we rely on explored loop's check.
            }
        }

        // Now delete states that were in explored (and not preserved)
        for (State* s : explored) {
            if (s != goal_state_to_preserve && s != initial_state_param) {
                // Check if it was already popped from frontier and added to frontier_states_to_delete
                // This avoids double delete if a state was in both and already handled by frontier pop.
                // However, direct deletion here is safer if we ensure frontier_states_to_delete doesn't include it.
                // A state from explored might not have been in the frontier_states_to_delete if it wasn't in frontier.
                bool already_in_frontier_delete_list = false;
                for (State* fs : frontier_states_to_delete) {
                    if (fs == s) {
                        already_in_frontier_delete_list = true;
                        break;
                    }
                }
                if (!already_in_frontier_delete_list) {
                    delete s;
                }
            }
        }
        explored.clear();

        // Now delete states collected from the frontier
        for (State* s : frontier_states_to_delete) {
            // No need to check for preservation again, already done when adding
            delete s;
        }
        // frontier is now empty.
    };

    frontier->add(initial_state_param);
    State *goal_found_state = nullptr;

    while (true) {
        iterations++;
        if (iterations % 10000 == 0) {
            printSearchStatus(explored, *frontier);
        }

        if (iterations % 1000 == 0) {
            if (Memory::getUsage() > Memory::maxUsage) {
                printSearchStatus(explored, *frontier);
                fprintf(stderr, "Maximum memory usage exceeded.\n");
                cleanupStates(); 
                return {};
            }
        }

        if (frontier->isEmpty()) {
            fprintf(stderr, "Frontier is empty.\n");
            cleanupStates(); 
            return {};
        }

        State *state = frontier->pop();

        if (state->isGoalState()) {
            printSearchStatus(explored, *frontier);
            goal_found_state = state; 
            std::vector<std::vector<const Action *>> plan = state->extractPlan();
            cleanupStates(goal_found_state); 
            if (goal_found_state != initial_state_param) {
                delete goal_found_state;
            }
            return plan;
        }

        explored.insert(state);

        for (State *child : state->getExpandedStates()) {
            if (explored.count(child) || frontier->contains(child)) { 
                delete child; 
            } else {
                frontier->add(child);
            }
        }
    }
}
