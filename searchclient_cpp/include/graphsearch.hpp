#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

#include "action.hpp"
#include "frontier.hpp"
#include "memory.hpp"
#include "state.hpp"

// Global start time
static auto start_time = std::chrono::high_resolution_clock::now();

void printSearchStatus(const std::unordered_set<State *> &explored, const Frontier &frontier) {
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        printf("#Expanded, Frontier, Generated, Time[s], Alloc[MB], MaxAlloc[MB]\n");
    }

    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration<double>(now - start_time).count();

    size_t size_explored = explored.size();
    size_t size_frontier = frontier.size();

    printf("#%8zu, %8zu, %9zu, %7.3f, %9.1f, %12.1f\n", size_explored, size_frontier, size_explored + size_frontier, elapsed_time,
           Memory::getUsage(), Memory::maxUsage);
}

std::vector<std::vector<const Action *>> search(State *initial_state, Frontier *frontier) {
    int iterations = 0;

    frontier->add(initial_state);
    std::unordered_set<State *> explored;

    while (true) {
        iterations++;
        if (iterations % 100 == 0) {  // 10000
            printSearchStatus(explored, *frontier);
        }

        if (Memory::getUsage() > Memory::maxUsage) {
            printSearchStatus(explored, *frontier);
            fprintf(stderr, "Maximum memory usage exceeded.\n");
            return {};  // Return an empty plan to indicate failure
        }

        if (frontier->isEmpty()) {
            fprintf(stderr, "Frontier is empty.\n");
            return {};  // Return an empty plan to indicate failure
        }

        State *state = frontier->pop();

        if (state->isGoalState()) {
            printSearchStatus(explored, *frontier);
            return state->extractPlan();
        }

        explored.insert(state);

        for (State *child : state->getExpandedStates()) {
            if (explored.find(child) == explored.end() && !frontier->contains(child)) {
                frontier->add(child);
            }
        }
    }
}
