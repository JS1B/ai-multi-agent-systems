// tests/test_level.cpp
#include <iostream>
#include <sstream>
#include <cassert>

#include "level.hpp"   // brings in CharGrid, Cell2D, WALL, loadLevel

void test_loadLevel() {
    // Prepare a fake .lvl file in a string
    const std::string lvl = 
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "MAExample\n"
        "#colors\n"
        "red: 0, A\n"
        "green: 1, B\n"
        "#initial\n"
        "+++++++\n"
        "+0BB  +\n"
        "+     +\n"
        "+1AA  +\n"
        "+++++++\n"
        "#goal\n"
        "+++++++\n"
        "+1  AA+\n"
        "+     +\n"
        "+0  BB+\n"
        "+++++++\n"
        "#end\n";

    std::istringstream in(lvl);
    Level level = loadLevel(in);

    // Domain & name (static members)
    assert(Level::domain == "hospital");
    assert(Level::name   == "MAExample");

    // Dimensions should be 5×7
    assert(Level::walls.size_rows() == 5);
    assert(Level::walls.size_cols() == 7);
    assert(level.boxes.size_rows() == 5);
    assert(level.boxes.size_cols() == 7);

    // Walls: border must be '+', interior must be untouched (0)
    for(int r = 0; r < 5; ++r) {
        for(int c = 0; c < 7; ++c) {
            if(r==0||r==4||c==0||c==6)
                assert(Level::walls(r,c) == WALL);
            else
                assert(Level::walls(r,c) == 0);
        }
    }

    // Agents: there should be exactly two, at (1,1) for '0' and (3,1) for '1'
    assert(level.agents.size() == 2);
    assert(level.agents[0] == Cell2D(1,1));
    assert(level.agents[1] == Cell2D(3,1));

    // Agent colors:
    assert(Level::agent_colors[0] == Color::Red);
    assert(Level::agent_colors[1] == Color::Green);

    // Box colors:
    assert(Level::box_colors[0] == Color::Red);
    assert(Level::box_colors[1] == Color::Green);

    // Boxes in initial state: B at (1,2),(1,3); A at (3,2),(3,3)
    assert(level.boxes(1,2) == 'B');
    assert(level.boxes(1,3) == 'B');
    assert(level.boxes(3,2) == 'A');
    assert(level.boxes(3,3) == 'A');
    // Check some empty cell
    assert(level.boxes(2,2) == 0);
    assert(level.boxes(2,3) == 0);
    assert(level.boxes(4,2) == 0);
    assert(level.boxes(4,3) == 0);
    assert(level.boxes(3,4) == 0);
    assert(level.boxes(3,5) == 0);

    // Goals: static Level::goals must have '1' at (1,1), 'A' at (1,4),(1,5);
    //                     '0' at (3,1), 'B' at (3,4),(3,5)
    assert(Level::agent_goals[1] == Cell2D(1,1));
    assert(Level::box_goals(1,4) == 'A');
    assert(Level::box_goals(1,5) == 'A');
    assert(Level::agent_goals[0] == Cell2D(3,1));
    assert(Level::box_goals(3,4) == 'B');
    assert(Level::box_goals(3,5) == 'B');
    // And interior zeros
    assert(Level::box_goals(2,2) == 0);
    assert(Level::box_goals(1,2) == 0);
    assert(Level::box_goals(1,3) == 0);
    assert(Level::box_goals(3,2) == 0);
    assert(Level::box_goals(3,3) == 0);

    std::cout << "test_loadLevel passed!" << std::endl;
}

int main() {
    test_loadLevel();
    return 0;
}
