#include <cassert>
#include <iostream>
#include <sstream>

#include "action.hpp"
#include "frontier.hpp"
#include "graphsearch.hpp"
#include "level.hpp"

Level loadGraphsearchLevel() {
    const std::string lvl =
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "graphsearch01\n"
        "#colors\n"
        "blue: 0\n"
        "#initial\n"
        "+++++\n"
        "+0  +\n"
        "+++++\n"
        "#goal\n"
        "+++++\n"
        "+ 0 +\n"
        "+++++\n"
        "#end\n";

    std::istringstream in(lvl);
    return loadLevel(in);
}

void testUnconstrainedSolve() {
    Level level = loadGraphsearchLevel();
    LowLevelState state(level.static_level, level.agents, level.boxes);
    Graphsearch graphsearch(&state, new FrontierBestFirst(new HeuristicAStar()));

    const std::vector<std::vector<const Action *>> plan = graphsearch.solve({});

    assert(graphsearch.wasSolutionFound());
    assert(!plan.empty());
    for (const std::vector<const Action *> &joint_action : plan) {
        state.applyActions(joint_action);
    }
    assert(state.isGoalState());

    std::cout << "testUnconstrainedSolve passed!" << std::endl;
}

void testConstraintIndexedByTimestep() {
    Level level = loadGraphsearchLevel();
    LowLevelState state(level.static_level, level.agents, level.boxes);
    Graphsearch graphsearch(&state, new FrontierBestFirst(new HeuristicAStar()));

    const std::vector<Constraint> constraints = {Constraint(Cell2D(1, 2), 1)};
    const std::vector<std::vector<const Action *>> plan = graphsearch.solve(constraints);

    assert(graphsearch.wasSolutionFound());
    assert(!plan.empty());
    state.applyActions(plan[0]);
    assert(state.agents[0].getPosition() != Cell2D(1, 2));
    for (size_t i = 1; i < plan.size(); ++i) {
        state.applyActions(plan[i]);
    }
    assert(state.isGoalState());

    std::cout << "testConstraintIndexedByTimestep passed!" << std::endl;
}

int main() {
    testUnconstrainedSolve();
    testConstraintIndexedByTimestep();
    return 0;
}
