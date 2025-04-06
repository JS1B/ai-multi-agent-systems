#pragma once

#include <string>

enum class ActionType {
    NoOp,
    Move,
    Push,
    Pull
};

class Action {
public:
    // Action types
    static const Action NoOp;
    
    // Move actions
    static const Action MoveN;
    static const Action MoveS;
    static const Action MoveE;
    static const Action MoveW;
    
    // Push actions
    static const Action PushNN;
    static const Action PushNE;
    static const Action PushNW;
    static const Action PushSS;
    static const Action PushSE;
    static const Action PushSW;
    static const Action PushEE;
    static const Action PushEN;
    static const Action PushES;
    static const Action PushWW;
    static const Action PushWN;
    static const Action PushWS;
    
    // Pull actions
    static const Action PullNN;
    static const Action PullNE;
    static const Action PullNW;
    static const Action PullSS;
    static const Action PullSE;
    static const Action PullSW;
    static const Action PullEE;
    static const Action PullEN;
    static const Action PullES;
    static const Action PullWW;
    static const Action PullWN;
    static const Action PullWS;

    // Member variables
    const std::string name;
    const ActionType type;
    const int agentRowDelta;  // vertical displacement of agent (-1,0,+1)
    const int agentColDelta;  // horizontal displacement of agent (-1,0,+1)
    const int boxRowDelta;    // vertical displacement of box (-1,0,+1)
    const int boxColDelta;    // horizontal displacement of box (-1,0,+1)

    // Constructor
    Action(const std::string& name, ActionType type, int ard, int acd, int brd, int bcd)
        : name(name), type(type), agentRowDelta(ard), agentColDelta(acd),
          boxRowDelta(brd), boxColDelta(bcd) {}

    // Comparison operators
    bool operator==(const Action& other) const {
        return //name == other.name && // Checking for name is not needed 
            type == other.type &&
            agentRowDelta == other.agentRowDelta &&
            agentColDelta == other.agentColDelta &&
            boxRowDelta == other.boxRowDelta &&
            boxColDelta == other.boxColDelta;
    }

    bool operator!=(const Action& other) const {
        return !(*this == other);
    }
};

// Define all static members
const Action Action::NoOp("NoOp", ActionType::NoOp, 0, 0, 0, 0);

const Action Action::MoveN("Move(N)", ActionType::Move, -1, 0, 0, 0);
const Action Action::MoveS("Move(S)", ActionType::Move, 1, 0, 0, 0);
const Action Action::MoveE("Move(E)", ActionType::Move, 0, 1, 0, 0);
const Action Action::MoveW("Move(W)", ActionType::Move, 0, -1, 0, 0);

const Action Action::PushNN("Push(N,N)", ActionType::Push, -1, 0, -1, 0);
const Action Action::PushNE("Push(N,E)", ActionType::Push, -1, 0, 0, 1);
const Action Action::PushNW("Push(N,W)", ActionType::Push, -1, 0, 0, -1);
const Action Action::PushSS("Push(S,S)", ActionType::Push, 1, 0, 1, 0);
const Action Action::PushSE("Push(S,E)", ActionType::Push, 1, 0, 0, 1);
const Action Action::PushSW("Push(S,W)", ActionType::Push, 1, 0, 0, -1);
const Action Action::PushEE("Push(E,E)", ActionType::Push, 0, 1, 0, 1);
const Action Action::PushEN("Push(E,N)", ActionType::Push, 0, 1, -1, 0);
const Action Action::PushES("Push(E,S)", ActionType::Push, 0, 1, 1, 0);
const Action Action::PushWW("Push(W,W)", ActionType::Push, 0, -1, 0, -1);
const Action Action::PushWN("Push(W,N)", ActionType::Push, 0, -1, -1, 0);
const Action Action::PushWS("Push(W,S)", ActionType::Push, 0, -1, 1, 0);

const Action Action::PullNN("Pull(N,N)", ActionType::Pull, -1, 0, -1, 0);
const Action Action::PullNE("Pull(N,E)", ActionType::Pull, -1, 0, 0, 1);
const Action Action::PullNW("Pull(N,W)", ActionType::Pull, -1, 0, 0, -1);
const Action Action::PullSS("Pull(S,S)", ActionType::Pull, 1, 0, 1, 0);
const Action Action::PullSE("Pull(S,E)", ActionType::Pull, 1, 0, 0, 1);
const Action Action::PullSW("Pull(S,W)", ActionType::Pull, 1, 0, 0, -1);
const Action Action::PullEE("Pull(E,E)", ActionType::Pull, 0, 1, 0, 1);
const Action Action::PullEN("Pull(E,N)", ActionType::Pull, 0, 1, -1, 0);
const Action Action::PullES("Pull(E,S)", ActionType::Pull, 0, 1, 1, 0);
const Action Action::PullWW("Pull(W,W)", ActionType::Pull, 0, -1, 0, -1);
const Action Action::PullWN("Pull(W,N)", ActionType::Pull, 0, -1, -1, 0);
const Action Action::PullWS("Pull(W,S)", ActionType::Pull, 0, -1, 1, 0);