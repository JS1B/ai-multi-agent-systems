#ifndef SEARCHCLIENT_STATE_H
#define SEARCHCLIENT_STATE_H

#include "action.h"
#include "color.h"

#include <vector>
#include <string>

class State
{
public:
    // Default constructor
    State() = default;

    /*
        The agent rows, columns, and colors are indexed by the agent number.
        For example, this->agentRows[0] is the row location of agent '0'.
    */

    static std::vector<int> agentRows;
    static std::vector<int> agentCols;
    static std::vector<Color> agentColors;

    /*
        The walls, boxes, and goals arrays are indexed from the top-left of the level, row-major order (row, col).
               Col 0  Col 1  Col 2  Col 3
        Row 0: (0,0)  (0,1)  (0,2)  (0,3)  ...
        Row 1: (1,0)  (1,1)  (1,2)  (1,3)  ...
        Row 2: (2,0)  (2,1)  (2,2)  (2,3)  ...
        ...

        For example, this->walls[2] is an array of booleans for the third row.
        this->walls[row][col] is true if there's a wall at (row, col).
    */
    static std::vector<std::vector<bool>> walls;
    static std::vector<std::vector<char>> boxes;
    static std::vector<Color> boxColors;
    static std::vector<std::vector<char>> goals;

    State *parent;
    std::vector<Action> jointAction;
    int g;

    State(std::vector<int> agentRows, std::vector<int> agentCols, std::vector<Color> agentColors,
          std::vector<std::vector<bool>> walls, std::vector<std::vector<char>> boxes, std::vector<Color> boxColors,
          std::vector<std::vector<char>> goals);

    std::vector<std::vector<Action>> extractPlan();
    std::vector<State> getExpandedStates();
    bool isGoalState();
    int getG() const;

    // Overload the == operator to compare two states
    bool operator==(const State &other) const;

    // Overload the < operator to compare two states (needed for set)
    bool operator<(const State &other) const;

    bool isApplicable(int agent, const Action &action);
    bool isConflicting(const std::vector<Action> &jointAction);
    State result(const std::vector<Action> &jointAction);

private:
    bool cellIsFree(int row, int col);
    char agentAt(int row, int col);
};

#endif // SEARCHCLIENT_STATE_H