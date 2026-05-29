#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

#include "action.hpp"
#include "level.hpp"
#include "low_level_state.hpp"

Level loadTestLevel(const std::string &level_text) {
    std::istringstream in(level_text);
    return loadLevel(in);
}

void deleteStates(std::vector<LowLevelState *> &states) {
    for (LowLevelState *state : states) {
        delete state;
    }
    states.clear();
}

void testGoalStateUsesCurrentModel() {
    const std::string lvl =
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "basic01\n"
        "#colors\n"
        "blue: 0, A\n"
        "#initial\n"
        "+++++\n"
        "+0 A+\n"
        "+   +\n"
        "+++++\n"
        "#goal\n"
        "+++++\n"
        "+   +\n"
        "+A  +\n"
        "+++++\n"
        "#end\n";

    Level level = loadTestLevel(lvl);
    LowLevelState state(level.static_level, level.agents, level.boxes);

    assert(!state.isGoalState());

    state.box_bulks[0].position(0) = Cell2D(2, 1);
    assert(state.isGoalState());

    std::cout << "testGoalStateUsesCurrentModel passed!" << std::endl;
}

void testPushApplicabilityAndApplication() {
    const std::string lvl =
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "push01\n"
        "#colors\n"
        "blue: 0, A\n"
        "#initial\n"
        "+++++\n"
        "+0A +\n"
        "+   +\n"
        "+++++\n"
        "#goal\n"
        "+++++\n"
        "+  A+\n"
        "+   +\n"
        "+++++\n"
        "#end\n";

    Level level = loadTestLevel(lvl);
    LowLevelState state(level.static_level, level.agents, level.boxes);

    assert(!state.isApplicable({&Action::MoveN}));
    assert(!state.isApplicable({&Action::MoveE}));
    assert(state.isApplicable({&Action::PushEE}));

    state.applyActions({&Action::PushEE});
    assert(state.agents[0].getPosition() == Cell2D(1, 2));
    assert(state.box_bulks[0].getPosition(0) == Cell2D(1, 3));
    assert(state.isGoalState());

    std::cout << "testPushApplicabilityAndApplication passed!" << std::endl;
}

void testExpansionCanReachImmediateGoal() {
    const std::string lvl =
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "pushExpansion01\n"
        "#colors\n"
        "blue: 0, A\n"
        "#initial\n"
        "+++++\n"
        "+0A +\n"
        "+   +\n"
        "+++++\n"
        "#goal\n"
        "+++++\n"
        "+  A+\n"
        "+   +\n"
        "+++++\n"
        "#end\n";

    Level level = loadTestLevel(lvl);
    LowLevelState state(level.static_level, level.agents, level.boxes);
    std::vector<LowLevelState *> expanded_states = state.getExpandedStates();

    bool found_goal_child = false;
    for (const LowLevelState *child : expanded_states) {
        found_goal_child = found_goal_child || child->isGoalState();
    }
    deleteStates(expanded_states);

    assert(found_goal_child);
    std::cout << "testExpansionCanReachImmediateGoal passed!" << std::endl;
}

int main() {
    testGoalStateUsesCurrentModel();
    testPushApplicabilityAndApplication();
    testExpansionCanReachImmediateGoal();
    return 0;
}
