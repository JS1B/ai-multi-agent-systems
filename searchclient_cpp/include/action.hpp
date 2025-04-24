#pragma once

#include <string>
#include <vector>

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

   private:
    // Constructor for private use only
    Action(const std::string &name, ActionType type, int ard, int acd, int brd, int bcd)
        : name(name), type(type), agentRowDelta(ard), agentColDelta(acd), boxRowDelta(brd), boxColDelta(bcd) {}

   public:
    // Comparison operators
    bool operator==(const Action &other) const {
        return  // name == other.name && // Checking for name is not needed
            type == other.type &&
            agentRowDelta == other.agentRowDelta &&
            agentColDelta == other.agentColDelta &&
            boxRowDelta == other.boxRowDelta &&
            boxColDelta == other.boxColDelta;
    }

    bool operator!=(const Action &other) const {
        return !(*this == other);
    }

    static const std::vector<const Action *> &values() {
        // This static local variable is initialized only once (thread-safe since C++11)
        // It holds pointers to all the static Action instances defined below.
        static const std::vector<const Action *> allActions = {
            &Action::NoOp,
            &Action::MoveN, &Action::MoveS, &Action::MoveE, &Action::MoveW,
            &Action::PushNN, &Action::PushNE, &Action::PushNW,
            &Action::PushSS, &Action::PushSE, &Action::PushSW,
            &Action::PushEE, &Action::PushEN, &Action::PushES,
            &Action::PushWW, &Action::PushWN, &Action::PushWS,
            &Action::PullNN, &Action::PullNE, &Action::PullNW,
            &Action::PullSS, &Action::PullSE, &Action::PullSW,
            &Action::PullEE, &Action::PullEN, &Action::PullES,
            &Action::PullWW, &Action::PullWN, &Action::PullWS};
        return allActions;
    }

    // Prevent copying and assignment if Actions should be treated like singletons
    // Action(const Action &) = delete;
    // Action &operator=(const Action &) = delete;
};
