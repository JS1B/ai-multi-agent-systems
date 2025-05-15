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

std::string formatJointAction(const std::vector<const Action *> &joint_action) {
    std::string result;
    if (joint_action.empty()) {
        return result;
    }

    size_t required_capacity = 0;
    for (const auto &action_ptr : joint_action) {
        if (action_ptr) {
            required_capacity += action_ptr->name.length();
        }
    }
    if (joint_action.size() > 1) {
        required_capacity += joint_action.size() - 1;
    }
    if (required_capacity > 0) {
        result.reserve(required_capacity);
    }

    for (size_t i = 0; i < joint_action.size(); ++i) {
        if (joint_action[i]) {
            result += joint_action[i]->name;
        }
        if (i < joint_action.size() - 1) {
            result += "|";
        }
    }
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
    fprintf(stderr, "Debug Level Info:\n");
    fprintf(stderr, "  Domain: '%s', Name: '%s'\n", level.getDomain().c_str(), level.getName().c_str());
    fprintf(stderr, "  Rows: %d, Cols: %d\n", level.getRows(), level.getCols());
    fprintf(stderr, "  Walls size: %zu (first row size: %zu)\n", 
            level.getWalls().size(), 
            (level.getRows() > 0 && !level.getWalls().empty()) ? level.getWalls()[0].size() : 0);
    fprintf(stderr, "  AgentsMap size: %zu, BoxesMap size: %zu, GoalsMap size: %zu\n", 
            level.agentsMap.size(), level.boxesMap.size(), level.getGoalsMap().size());
    fprintf(stderr, "  SortedAgentIds size: %zu, SortedBoxIds size: %zu\n", 
            level.getSortedAgentIds().size(), level.getSortedBoxIds().size());
    if (!level.getSortedAgentIds().empty()) {
        fprintf(stderr, "  First sorted agent ID: %c\n", level.getSortedAgentIds()[0]);
    }
     if (!level.getSortedBoxIds().empty()) {
        fprintf(stderr, "  First sorted box ID: %c\n", level.getSortedBoxIds()[0]);
    }
    fprintf(stderr, "End Debug Level Info.\n");

    // Create initial state
    // State *initial_state = new State(level);
    State initial_state_obj(level); // Stack allocate State
    State* initial_state = &initial_state_obj; // Use pointer for existing interfaces

    fprintf(stderr, "DEBUG: initial_state pointer (to stack obj) = %p\n", (void*)initial_state);
    if (initial_state) {
        fprintf(stderr, "DEBUG: initial_state agents size = %zu (expected %zu)\n", 
                initial_state->currentAgents_.size(), level.agentsMap.size());
    }

    // Create Heuristic Calculator instance
    std::unique_ptr<HeuristicCalculators::HeuristicCalculator> calculator;
    if (heuristic_calc_name == "mds") {
        calculator = std::make_unique<HeuristicCalculators::ManhattanDistanceCalculator>(level);
        fprintf(stderr, "DEBUG: Directly after creating MDC, calculator.get() = %p\n", (void*)calculator.get());
        if (calculator) {
             fprintf(stderr, "DEBUG: MDC Name via direct unique_ptr: %s\n", calculator->getName().c_str());
        }
    } else if (heuristic_calc_name == "sic") {
        calculator = std::make_unique<HeuristicCalculators::SumOfIndividualCostsCalculator>(level);
        fprintf(stderr, "DEBUG: Directly after creating SIC, calculator.get() = %p\n", (void*)calculator.get());
        if (calculator) {
             fprintf(stderr, "DEBUG: SIC Name via direct unique_ptr: %s\n", calculator->getName().c_str());
        }
    } else if (heuristic_calc_name == "bgm") {
        calculator = std::make_unique<HeuristicCalculators::BoxGoalManhattanDistanceCalculator>(level);
        // Add similar debug prints if needed for BGM
    } else if (heuristic_calc_name == "zero") {
        calculator = std::make_unique<HeuristicCalculators::ZeroHeuristicCalculator>(level);
        // Add similar debug prints if needed for Zero
    } else if (heuristic_calc_name == "scp") {
        calculator = std::make_unique<SCPHeuristicCalculator>(level);
        // Add similar debug prints if needed for SCP
    } else {
        fprintf(stderr, "Warning: Unknown heuristic calculator '%s'. Defaulting to ZeroHeuristic.\n", heuristic_calc_name.c_str());
        calculator = std::make_unique<HeuristicCalculators::ZeroHeuristicCalculator>(level);
        fprintf(stderr, "DEBUG: Directly after creating Zero (default), calculator.get() = %p\n", (void*)calculator.get());
        if (calculator) {
             fprintf(stderr, "DEBUG: Zero (default) Name via direct unique_ptr: %s\n", calculator->getName().c_str());
        }
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
        // delete initial_state; // No longer deleting initial_state if it was stack based and factory failed early
        return 1;
    }

    // Prepare heuristic info string for logging
    std::string heuristic_info_str = "N/A";
    FrontierBestFirst* best_first_frontier_ptr = dynamic_cast<FrontierBestFirst*>(frontier);
    if (best_first_frontier_ptr) {
        const Heuristic* heuristic_obj_ptr = best_first_frontier_ptr->getHeuristic();
        fprintf(stderr, "DEBUG: best_first_frontier_ptr = %p, heuristic_obj_ptr = %p\n", (void*)best_first_frontier_ptr, (const void*)heuristic_obj_ptr);
        if (heuristic_obj_ptr) {
            heuristic_info_str = heuristic_obj_ptr->getCalculatorName();
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
    for (const auto &joint_action_vec : plan) {
        s = formatJointAction(joint_action_vec);

        fprintf(stdout, "%s\n", s.c_str());
        fflush(stdout);

        // Read server's response to not fill up the stdin buffer and block the server.
        std::string response;
        getline(std::cin, response);
        // fprintf(stderr, "Client: Sent action: %s. Server response: %s\n", s.c_str(), response.c_str()); // Print response
    }
#endif

    delete frontier;
    // delete initial_state; // initial_state is now a pointer to a stack object, no delete needed
    return 0;
}