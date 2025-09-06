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
// #include "action.hpp"
#include "cbs.hpp"
// #include "color.hpp"
#include "feature_flags.hpp"
// #include "frontier.hpp"
// #include "graphsearch.hpp"
// #include "helpers.hpp"
// #include "heuristic.hpp"
#include "level.hpp"

/*
For a text to be treated as a comment, it must be sent via:
- stderr - all messages
- stdout - starting with #
*/

Level *level_instance = nullptr;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "C++ SearchClient initializing.\n");

    // Send client name to server.
    fprintf(stdout, "SearchClient\n");

    fprintf(stderr, "Feature flags: %s\n", getFeatureFlags());

    level_instance = new Level(loadLevel(std::cin));
    fprintf(stderr, "Loaded %s\n", level_instance->toString().c_str());

    fprintf(stderr, "Starting CBS...\n");
    CBS *cbs = new CBS(*level_instance);
    std::vector<std::vector<const Action *>> plan = cbs->solve();
    fflush(stdout);
    fflush(stderr);

    // Print plan to server
    if (plan.empty()) {
        fprintf(stderr, "Unable to solve level.\n");
        delete cbs;
        delete level_instance;
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
    }
#endif
    return 0;
}