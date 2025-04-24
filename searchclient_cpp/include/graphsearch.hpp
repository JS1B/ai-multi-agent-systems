#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>

#include "action.hpp"
#include "frontier.hpp"
#include "memory.hpp"
#include "state.hpp"

using namespace std;

// Global start time
static auto start_time = chrono::high_resolution_clock::now();

void printSearchStatus(const set<State> &explored, Frontier *frontier) {
    auto now = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration<double>(now - start_time).count();

    printf("#Expanded: %8zu, Frontier: %8zu, Generated: %8zu, Time: %.3f s\n",
           explored.size(), frontier->size(), explored.size() + frontier->size(), elapsed_time);
    printf("#[Alloc: %.2f MB, MaxAlloc: %.2f MB]\n",
           Memory::getUsage(), Memory::maxUsage);
}

std::vector<std::vector<Action>> search(State initial_state, Frontier *frontier) {
    // bool output_fixed_solution = false;
    // if (output_fixed_solution) {
    //     return {
    //         {Action::MoveS},
    //         {Action::MoveS},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveE},
    //         {Action::MoveS},
    //         {Action::MoveS}};
    // }

    int iterations = 0;

    frontier->add(initial_state);
    set<State> explored;

    while (true) {
        iterations++;
        if (iterations % 10000 == 0) {
            printSearchStatus(explored, frontier);
        }

        if (Memory::getUsage() > Memory::maxUsage) {
            printSearchStatus(explored, frontier);
            fprintf(stderr, "Maximum memory usage exceeded.\n");
            return {};  // Return an empty plan to indicate failure
        }

        if (frontier->isEmpty()) {
            fprintf(stderr, "Frontier is empty.\n");
            return {};  // Return an empty plan to indicate failure
        }

        State state = frontier->pop();

        if (state.isGoalState()) {
            printSearchStatus(explored, frontier);
            return state.extractPlan();
        }

        explored.insert(state);

        for (State child : state.getExpandedStates()) {
            if (explored.find(child) == explored.end() && !frontier->contains(child)) {
                frontier->add(child);
            }
        }
    }
}
