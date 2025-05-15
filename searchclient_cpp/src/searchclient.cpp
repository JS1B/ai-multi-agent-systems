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
#include "HeuristicCalculator.hpp"
#include "level.hpp"
#include "state.hpp"
#include "SCPHeuristicCalculator.hpp"

/*
For a text to be treated as a comment, it must be sent via:
- stderr - all messages
- stdout - starting with #
*/

std::string formatJointAction(const std::vector<const Action *> &joint_action, bool with_bubble = true) {
    static const size_t max_action_string_length = 20;

    std::string result;
    result.reserve(joint_action.size() * max_action_string_length);

    for (const auto &action : joint_action) {
        result += action->name + (with_bubble ? "@" + action->name : "");
        result += "|";
    }
    result.pop_back();
    return result;
}

// Map string to strategy factory functions
// These will now take an initial_state and a heuristic_calculator
using HeuristicStrategyFactory = std::function<Frontier*(State*, std::unique_ptr<HeuristicCalculators::HeuristicCalculator>)>;
std::unordered_map<std::string, HeuristicStrategyFactory> strategy_factory_map = {
    {"bfs", [](State* /*initial_state*/, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> /*calc*/) {
        return new FrontierBFS();
    }},
    {"dfs", [](State* /*initial_state*/, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> /*calc*/) {
        return new FrontierDFS();
    }},
    {"astar", [](State* initial_state, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc) {
        return new FrontierBestFirst(new HeuristicAStar(*initial_state, std::move(calc)));
    }},
    {"wastar", [](State* initial_state, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc) {
        // Default weight for WA*, can be made configurable too
        int weight = 3; 
        return new FrontierBestFirst(new HeuristicWeightedAStar(*initial_state, weight, std::move(calc)));
    }},
    {"greedy", [](State* initial_state, std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calc) {
        return new FrontierBestFirst(new HeuristicGreedy(*initial_state, std::move(calc)));
    }}
};

int main(int argc, char *argv[]) {
    fprintf(stderr, "C++ SearchClient initializing.\n");

    // Send client name to server. Try matching Java client's class name.
    fprintf(stdout, "searchclient.SearchClient\n");
    fflush(stdout); // Ensure it's sent immediately

    fprintf(stderr, "Feature flags: %s\n", getFeatureFlags());

    // Default values
    std::string strategy_name = "bfs";
    std::string heuristic_calc_name = "zero"; // Default heuristic for A*/Greedy

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s") {
            if (i + 1 < argc) { // Make sure there's another argument for the strategy name
                strategy_name = argv[++i]; // Use the next argument as strategy_name and increment i
            } else {
                fprintf(stderr, "Error: -s option requires a strategy name.\n");
                return 1;
            }
        } else if (arg == "-hc") {
            if (i + 1 < argc) { // Make sure there's another argument for the heuristic calculator name
                heuristic_calc_name = argv[++i]; // Use the next argument as heuristic_calc_name and increment i
            } else {
                fprintf(stderr, "Error: -hc option requires a heuristic calculator name.\n");
                return 1;
            }
        } else {
            // If it's an argument we don't recognize, print a warning.
            // You could also make this an error and print usage details.
            fprintf(stderr, "Warning: Unknown argument: %s\n", arg.c_str());
        }
    }
    
    fprintf(stderr, "Strategy: %s, Heuristic Calculator: %s\n", strategy_name.c_str(), heuristic_calc_name.c_str());

    // Parse the level from stdin
    Level level = loadLevel(std::cin);
    fprintf(stderr, "Loaded %s\n", level.toString().c_str());

    // Create initial state
    State *initial_state = new State(level);

    // Create Heuristic Calculator instance
    std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calculator;
    if (heuristic_calc_name == "mds") {
        calculator = std::make_unique<HeuristicCalculators::ManhattanDistanceCalculator>(level);
    } else if (heuristic_calc_name == "sic") {
        calculator = std::make_unique<HeuristicCalculators::SumOfIndividualCostsCalculator>(level);
    } else if (heuristic_calc_name == "bgm") {
        calculator = std::make_unique<HeuristicCalculators::BoxGoalManhattanDistanceCalculator>(level);
    } else if (heuristic_calc_name == "zero") {
        calculator = std::make_unique<HeuristicCalculators::ZeroHeuristicCalculator>(level);
    } else if (heuristic_calc_name == "scp") {
        calculator = std::make_unique<SCPHeuristicCalculator>(level);
    } else {
        fprintf(stderr, "Warning: Unknown heuristic calculator '%s'. Defaulting to ZeroHeuristic.\n", heuristic_calc_name.c_str());
        calculator = std::make_unique<HeuristicCalculators::ZeroHeuristicCalculator>(level);
    }

    Frontier *frontier;
    try {
        // Get the factory function from the map
        auto factory_it = strategy_factory_map.find(strategy_name);
        if (factory_it == strategy_factory_map.end()) {
            throw std::runtime_error("Unknown strategy: " + strategy_name);
        }
        // Call the factory, passing the initial_state and the chosen calculator
        frontier = factory_it->second(initial_state, std::move(calculator));
    } catch (const std::exception &e) {
        fprintf(stderr, "Error initializing strategy: %s\n", e.what());
        delete initial_state; // Clean up initial_state if frontier creation fails
        return 1;
    }

    // Prepare heuristic info string for logging
    std::string heuristic_info_str = "N/A";
    FrontierBestFirst* best_first_frontier_ptr = dynamic_cast<FrontierBestFirst*>(frontier);
    if (best_first_frontier_ptr) {
        const Heuristic* heuristic_ptr = best_first_frontier_ptr->getHeuristic();
        if (heuristic_ptr) {
            heuristic_info_str = heuristic_ptr->getCalculatorName();
        }
    }

    fprintf(stderr, "Starting search with %s using %s heuristic calculation.\n", 
            frontier->getName().c_str(), 
            heuristic_info_str.c_str());

    // Search for a plan
    std::vector<std::vector<const Action *>> plan = search(initial_state, frontier);

    // Print plan to server
    if (plan.empty()) {
        fprintf(stderr, "Unable to solve level.\n");
        return 0;
    }

    fprintf(stderr, "Found solution of length %zu.\n", plan.size());

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
        // fprintf(stderr, "Client: Sent action: %s. Server response: %s\n", s.c_str(), response.c_str()); // Print response
    }
#endif

    delete frontier;
    delete initial_state;
    return 0;
}