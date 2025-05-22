#pragma once

#include <array>
#include <string>
#include <vector>

//#include "point2d.hpp"
#include "cell2d.hpp"

enum class ActionType { NoOp, Move, Push, Pull };

class Action {
   private:
    Action(const std::string &name, const ActionType type, const Cell2D agent_delta, const Cell2D box_delta);

   public:
    Action(const Action &) = delete;
    Action &operator=(const Action &) = delete;

    // Member variables
    const std::string name;
    const ActionType type;
    const Cell2D agent_delta;
    const Cell2D box_delta;

    // Comparison operators
    bool operator==(const Action &other) const {
        //return type == other.type && agent_delta == other.agent_delta && box_delta == other.box_delta;
        return agent_delta == other.agent_delta && box_delta == other.box_delta; // this is enough, checking the type is redundant
    }

    bool operator!=(const Action &other) const { return !(*this == other); }

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

    static const std::array<const Action *, 29> &allValues();
};

std::string formatJointAction(const std::vector<const Action *> &joint_action, bool with_bubble = true);