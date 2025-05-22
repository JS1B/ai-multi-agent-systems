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

std::vector<std::vector<const Action *>> search(State *initial_state, Frontier *frontier) {
    int iterations = 0;

    frontier->add(initial_state);
    std::unordered_set<State *, StatePtrHash, StatePtrEqual> explored;
    explored.reserve(400'000);

    while (true) {
        iterations++;
        if (iterations % 10000 == 0) {  // 10000
            printSearchStatus(explored, *frontier);
        }

        if (iterations % 1000 == 0) {
            if (Memory::getUsage() > Memory::maxUsage) {
                printSearchStatus(explored, *frontier);
                fprintf(stderr, "Maximum memory usage exceeded.\n");
                return {};  // Return an empty plan to indicate failure
            }
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

        //std::cerr << "Expanded states: " << state->getExpandedStates().size() << std::endl;
        for (State *child : state->getExpandedStates()) {
            //std::cerr << "Child:\n" << child->toString() << std::endl;

            /*
            // Detect the state where box disappears
            if (child->level.boxes(child->level.agents[0]) != 0) {
                std::cerr << "Box disappears!" << std::endl;
                std::cerr << "Action:\n" << formatJointAction(child->jointAction, false) << std::endl;
                std::cerr << "Child box:\n" << child->toString() << std::endl;
            }
            */

            if (false &&child->level.boxes(2, 1) != 0) {
                std::cerr << "This should be goal state!" << std::endl;
                std::cerr << "Action:\n" << formatJointAction(child->jointAction, false) << std::endl;
                std::cerr << "Is goal state: " << child->isGoalState() << std::endl;
                std::cerr << "Child:\n" << child->toString() << std::endl;
            }

            if (explored.find(child) == explored.end() && !frontier->contains(child)) {
                frontier->add(child);
                continue;
            }
            delete child;  // @todo: check if this is correct - may not be enough
        }
    }
}
