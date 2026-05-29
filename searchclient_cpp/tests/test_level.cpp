#include <cassert>
#include <iostream>
#include <sstream>

#include "level.hpp"

void testLoadLevelCurrentModel() {
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

    assert(level.static_level.getDomain() == "hospital");
    assert(level.static_level.getName() == "MAExample");
    assert(level.static_level.getSize() == Cell2D(5, 7));

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 7; ++col) {
            const bool isBorder = row == 0 || row == 4 || col == 0 || col == 6;
            assert(level.static_level.isCellFree(Cell2D(row, col)) == !isBorder);
        }
    }

    assert(level.agents.size() == 2);
    assert(level.agents[0].getSymbol() == '0');
    assert(level.agents[0].getPosition() == Cell2D(1, 1));
    assert(level.agents[0].getGoalPositions().size() == 1);
    assert(level.agents[0].getGoalPositions()[0] == Cell2D(3, 1));
    assert(level.static_level.getAgentColor('0') == Color::Red);

    assert(level.agents[1].getSymbol() == '1');
    assert(level.agents[1].getPosition() == Cell2D(3, 1));
    assert(level.agents[1].getGoalPositions().size() == 1);
    assert(level.agents[1].getGoalPositions()[0] == Cell2D(1, 1));
    assert(level.static_level.getAgentColor('1') == Color::Green);

    assert(level.boxes.size() == 2);
    assert(level.boxes[0].getSymbol() == 'A');
    assert(level.boxes[0].getColor() == Color::Red);
    assert(level.boxes[0].size() == 2);
    assert(level.boxes[0].getPosition(0) == Cell2D(3, 2));
    assert(level.boxes[0].getPosition(1) == Cell2D(3, 3));
    assert(level.boxes[0].getGoalsCount() == 2);
    assert(level.boxes[0].getGoal(0) == Cell2D(1, 4));
    assert(level.boxes[0].getGoal(1) == Cell2D(1, 5));

    assert(level.boxes[1].getSymbol() == 'B');
    assert(level.boxes[1].getColor() == Color::Green);
    assert(level.boxes[1].size() == 2);
    assert(level.boxes[1].getPosition(0) == Cell2D(1, 2));
    assert(level.boxes[1].getPosition(1) == Cell2D(1, 3));
    assert(level.boxes[1].getGoalsCount() == 2);
    assert(level.boxes[1].getGoal(0) == Cell2D(3, 4));
    assert(level.boxes[1].getGoal(1) == Cell2D(3, 5));

    std::cout << "testLoadLevelCurrentModel passed!" << std::endl;
}

void testOrphanBoxesBecomeStaticWalls() {
    const std::string lvl =
        "#domain\n"
        "hospital\n"
        "#levelname\n"
        "OrphanBox\n"
        "#colors\n"
        "blue: 0\n"
        "red: A\n"
        "#initial\n"
        "+++++\n"
        "+0A +\n"
        "+++++\n"
        "#goal\n"
        "+++++\n"
        "+  A+\n"
        "+++++\n"
        "#end\n";

    std::istringstream in(lvl);
    Level level = loadLevel(in);

    assert(level.agents.size() == 1);
    assert(level.boxes.empty());
    assert(!level.static_level.isCellFree(Cell2D(1, 2)));

    std::cout << "testOrphanBoxesBecomeStaticWalls passed!" << std::endl;
}

int main() {
    testLoadLevelCurrentModel();
    testOrphanBoxesBecomeStaticWalls();
    return 0;
}
