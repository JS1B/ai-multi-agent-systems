#include <cassert>
#include <iostream>

#include "state.hpp"
#include "cell2d.hpp"
#include "chargrid.hpp"

void test_is_cell_free() {
    // 1) Set up a 3×3 world
    Level::walls = CharGrid(3, 3);
    // put a wall at (1,1)
    Level::walls(1,1) = '+';

    // 2) Set up a boxes grid of the same size:
    CharGrid boxes(3, 3);
    // put a box at (0,2)
    boxes(0,2) = 'B';
    boxes(2,1) = 'A';

    // 3) One agent at (2,0)
    std::vector<Cell2D> agents = { Cell2D(2,0) };
    
    // 4) Build the Level & wrap in our TestState
    Level lvl(agents, boxes);
    State st(lvl);

    // 5) Now assert that walls, boxes, and agents are reported busy,
    //    and an empty cell is reported free:
    assert(!st.is_cell_free(Cell2D(1,1)) && "wall cell should be non-free");
    assert(!st.is_cell_free(Cell2D(0,2)) && "box cell should be non-free");
    assert(!st.is_cell_free(Cell2D(2,1)) && "box cell should be non-free");
    assert(!st.is_cell_free(Cell2D(2,0)) && "agent cell should be non-free");
    assert( st.is_cell_free(Cell2D(0,0)) && "empty cell should be free");
    assert( st.is_cell_free(Cell2D(2,2)) && "empty cell should be free");
    assert( st.is_cell_free(Cell2D(1,2)) && "empty cell should be free");

    std::cout << "test_is_cell_free passed!" << std::endl;
}

void test_is_goal_state() {
    // 1) Agents-only scenario
    {
        // 2×2 grid, two agents at their goals
        std::vector<Cell2D> agents = { {0,0}, {1,1} };
        CharGrid boxes(2,2);               // no boxes
        Level::agent_goals = std::vector<Cell2D>({ {0,0}, {1,1} });
        Level lvl(agents, boxes);
        State st(lvl);
        assert(st.isGoalState() && "Agents on their goals should be goal state");
    }
    {
        // Agent 0 off its goal → should not be goal state
        std::vector<Cell2D> agents = { {2,1}, {1,1} };
        CharGrid boxes(2,2);
        Level::agent_goals = std::vector<Cell2D>({ {1,2}, {1,1} });
        Level lvl(agents, boxes);
        State st(lvl);
        assert(!st.isGoalState() && "Agent off its goal should not be goal state");
    }

    // 2) Boxes-only scenario
    {
        std::vector<Cell2D> agents;        // no agents
        CharGrid boxes(2,2);
        boxes(1,0) = 'A';
        Level::box_goals = CharGrid(2,2);
        Level::box_goals(1,0) = 'A';
        Level lvl(agents, boxes);
        State st(lvl);
        assert(st.isGoalState() && "Box on its goal should be goal state");
    }
    {
        std::vector<Cell2D> agents;
        CharGrid boxes(2,2);
        boxes(1,0) = 'A';
        Level::box_goals = CharGrid(2,2);
        Level::box_goals(1,0) = 'A';
        // Move box off its goal
        boxes(1,0) = 'B';
        Level lvl(agents, boxes);
        State st(lvl);
        assert(!st.isGoalState() && "Box off its goal should not be goal state");
    }

    // 3) Combined agents and boxes
    {
        std::vector<Cell2D> agents = { {1,2} };
        CharGrid boxes(3,3);
        boxes(1,1) = 'C';
        Level::agent_goals = std::vector<Cell2D>({ {1, 2} });
        Level::box_goals = CharGrid(3,3);
        Level::box_goals(1,1) = 'C';
        Level lvl(agents, boxes);
        State st(lvl);
        assert(st.isGoalState() && "Both agent and box on goals -> goal state");
    }

    std::cout << "test_is_goal_state passed!" << std::endl;
}

void test_is_goal_basic01() {
    const std::string lvl_txt =
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

    // load the level
    std::istringstream in(lvl_txt);
    Level lvl = loadLevel(in); // Level resets static members
    State st(lvl);

    // In this initial configuration the agent is at (1,1) but its goal is not set,
    // and the box 'A' is at (1,3) whereas the goal for 'A' is at (2,1),
    // so this should NOT be a goal state.
    assert(!st.isGoalState() && "basic01 initial state should not be goal");

    lvl.boxes(1, 3) = 0;
    lvl.boxes(2, 1) = 'A';
    State st2(lvl);

    assert(st2.isGoalState() && "basic01 goal state should be goal");
}

int main() {
    test_is_cell_free();
    test_is_goal_state();
    test_is_goal_basic01();
    return 0;
}
