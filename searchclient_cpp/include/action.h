#ifndef SEARCHCLIENT_ACTION_H
#define SEARCHCLIENT_ACTION_H

#include <string>
#include <vector>
enum class ActionType
{
    NoOp = 0,
    Move = 1,
    Push = 2,
    Pull = 3
};

class Action
{
public:
    std::string name;
    ActionType type;
    int agentRowDelta;
    int agentColDelta;
    int boxRowDelta;
    int boxColDelta;

    Action(std::string name, ActionType type, int ard, int acd, int brd, int bcd);

    static const Action NoOp;
    static const Action MoveN;
    static const Action MoveS;
    static const Action MoveE;
    static const Action MoveW;

    static const std::vector<Action> ALL_ACTIONS;
};

#endif // SEARCHCLIENT_ACTION_H