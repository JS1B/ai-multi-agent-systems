#include "graphsearch.h"
#include "memory.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <set>
#include <vector>

using namespace std;

// Global start time
static auto start_time = chrono::high_resolution_clock::now();

vector<vector<Action>> search(State initial_state, Frontier *frontier)
{
    bool output_fixed_solution = true;

    if (output_fixed_solution)
    {
        // Part 1:
        // The agents will perform the sequence of actions returned by this method.
        // Try to solve a few levels by hand, enter the found solutions below, and run them:

        return {
            {Action::MoveS},
            {Action::MoveS},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveE},
            {Action::MoveS},
            {Action::MoveS}};
    }

    // Part 2:
    // Now try to implement the Graph-Search algorithm from R&N figure 3.7
    // In the case of "failure to find a solution" you should return None.
    // Some useful methods on the state class which you will need to use are:
    // state.is_goal_state() - Returns true if the state is a goal state.
    // state.extract_plan() - Returns the list of actions used to reach this state.
    // state.get_expanded_states() - Returns a list containing the states reachable from the current state.
    // You should also take a look at frontier.h to see which methods the Frontier interface exposes
    //
    // printSearchStatus(expanded, frontier): As you can see below, the code will print out status
    // (#expanded states, size of the frontier, #generated states, total time used) for every 1000th node
    // generated.
    // You should also make sure to print out these stats when a solution has been found, so you can keep
    // track of the exact total number of states generated!!

    int iterations = 0;

    frontier->add(initial_state);
    set<State> explored;

    while (true)
    {
        iterations++;
        if (iterations % 10000 == 0)
        {
            printSearchStatus(explored, frontier);
        }

        if (Memory::getUsage() > Memory::maxUsage)
        {
            printSearchStatus(explored, frontier);
            cerr << "Maximum memory usage exceeded." << endl;
            return {}; // Return an empty plan to indicate failure
        }

        if (frontier->isEmpty())
        {
            cerr << "Frontier is empty." << endl;
            return {}; // Return an empty plan to indicate failure
        }

        State state = frontier->pop();

        if (state.isGoalState())
        {
            printSearchStatus(explored, frontier);
            return state.extractPlan();
        }

        explored.insert(state);

        for (State child : state.getExpandedStates())
        {
            if (explored.find(child) == explored.end() && !frontier->contains(child))
            {
                frontier->add(child);
            }
        }
    }
}

void printSearchStatus(const set<State> &explored, Frontier *frontier)
{
    auto now = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration<double>(now - start_time).count();

    cout << "#Expanded: " << setw(8) << explored.size() << ", #Frontier: " << setw(8) << frontier->size()
         << ", #Generated: " << setw(8) << explored.size() + frontier->size() << ", Time: " << fixed
         << setprecision(3) << elapsed_time << " s" << endl
         << "[Alloc: " << fixed << setprecision(2) << Memory::getUsage() << " MB, MaxAlloc: "
         << Memory::maxUsage << " MB]" << endl;
}