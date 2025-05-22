// C++ related
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// C related
#include <cctype>

// Own code
#include "action.hpp"
#include "color.hpp"
#include "feature_flags.hpp"
#include "frontier.hpp"
#include "graphsearch.hpp"
#include "helpers.hpp"
#include "heuristic.hpp"
#include "level.hpp"
#include "state.hpp"

/*
For a text to be treated as a comment, it must be sent via:
- stderr - all messages
- stdout - starting with #
*/

// Map string to strategy
std::unordered_map<std::string, std::function<Frontier *()>> strategy_map = {
    {"bfs", []() { return new FrontierBFS(); }}, {"dfs", []() { return new FrontierDFS(); }},
    // {"astar", new FrontierBestFirst(new HeuristicAStar(initial_state))},
    // {"wastar", new FrontierBestFirst(new HeuristicGreedy(initial_state))},
};

int main(int argc, char *argv[]) {
    fprintf(stderr, "C++ SearchClient initializing.\n");

    // Send client name to server.
    fprintf(stdout, "SearchClient\n");

    fprintf(stderr, "Feature flags: %s\n", getFeatureFlags());

    // Parse command line arguments (e.g., for specifying search strategy)
    std::string strategy = "bfs";
    if (argc > 1) {
        strategy = std::string(argv[1]);
    }

    // Parse the level from stdin
    Level level = loadLevel(std::cin);
    fprintf(stderr, "Loaded %s\n", level.toString().c_str());

    // Create initial state
    State *initial_state = new State(level);

    // Create frontier
    Frontier *frontier;
    try {
        frontier = strategy_map.at(strategy)();
    } catch (const std::exception &e) {
        fprintf(stderr, "Unknown strategy: %s\n", strategy.c_str());
        return 1;
    }

    // Search for a plan
    fprintf(stderr, "Starting %s.\n", frontier->getName().c_str());
    std::vector<std::vector<const Action *>> plan = search(initial_state, frontier);

    // Print initial state:
    //fprintf(stderr, "Initial state:\n %s\n", initial_state->toString().c_str());
    std::cerr << "Initial state:" << std::endl;
    std::cerr << initial_state->toString() << std::endl;

    // Print plan to server
    if (plan.empty()) {
        fprintf(stderr, "Unable to solve level.\n");
        return 0;
    }

    fprintf(stderr, "Found solution of length %zu.\n", plan.size());
    for (const auto &joint_action : plan) {
        fprintf(stderr, "Plan: %s\n", formatJointAction(joint_action, false).c_str());
    }

#ifdef DISABLE_ACTION_PRINTING
#else
    std::string s;
    for (const auto &joint_action : plan) {
        s = formatJointAction(joint_action, false);

        fprintf(stdout, "%s\n", s.c_str());
        fflush(stdout);

        // Read server's response to not fill up the stdin buffer and block the server.
        std::string response;
        getline(std::cin, response);
    }
#endif

    delete frontier;
    delete initial_state;
    return 0;
}