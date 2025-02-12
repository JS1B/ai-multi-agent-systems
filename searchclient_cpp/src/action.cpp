#include "action.h"

Action::Action(std::string name, ActionType type, int ard, int acd, int brd, int bcd)
    : name(name), type(type), agentRowDelta(ard), agentColDelta(acd), boxRowDelta(brd), boxColDelta(bcd) {}

const Action Action::NoOp = Action("NoOp", ActionType::NoOp, 0, 0, 0, 0);
const Action Action::MoveN = Action("Move(N)", ActionType::Move, -1, 0, 0, 0);
const Action Action::MoveS = Action("Move(S)", ActionType::Move, 1, 0, 0, 0);
const Action Action::MoveE = Action("Move(E)", ActionType::Move, 0, 1, 0, 0);
const Action Action::MoveW = Action("Move(W)", ActionType::Move, 0, -1, 0, 0);

const std::vector<Action> Action::ALL_ACTIONS = {
    NoOp,
    MoveN,
    MoveS,
    MoveE,
    MoveW,
};