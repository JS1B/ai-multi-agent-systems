#include "action.hpp"

#include "point2d.hpp"

Action::Action(const std::string &name, const ActionType type, const Cell2D agent_delta, const Cell2D box_delta)
    : name(name), type(type), agent_delta(agent_delta), box_delta(box_delta) {}

const Action Action::NoOp("NoOp", ActionType::NoOp, {0, 0}, {0, 0});

const Action Action::MoveN("Move(N)", ActionType::Move, {-1, 0}, {0, 0});
const Action Action::MoveS("Move(S)", ActionType::Move, {1, 0}, {0, 0});
const Action Action::MoveE("Move(E)", ActionType::Move, {0, 1}, {0, 0});
const Action Action::MoveW("Move(W)", ActionType::Move, {0, -1}, {0, 0});

const Action Action::PushNN("Push(N,N)", ActionType::Push, {-1, 0}, {-1, 0});
const Action Action::PushNE("Push(N,E)", ActionType::Push, {-1, 0}, {0, 1});
const Action Action::PushNW("Push(N,W)", ActionType::Push, {-1, 0}, {0, -1});
const Action Action::PushSS("Push(S,S)", ActionType::Push, {1, 0}, {1, 0});
const Action Action::PushSE("Push(S,E)", ActionType::Push, {1, 0}, {0, 1});
const Action Action::PushSW("Push(S,W)", ActionType::Push, {1, 0}, {0, -1});
const Action Action::PushEE("Push(E,E)", ActionType::Push, {0, 1}, {0, 1});
const Action Action::PushEN("Push(E,N)", ActionType::Push, {0, 1}, {-1, 0});
const Action Action::PushES("Push(E,S)", ActionType::Push, {0, 1}, {1, 0});
const Action Action::PushWW("Push(W,W)", ActionType::Push, {0, -1}, {0, -1});
const Action Action::PushWN("Push(W,N)", ActionType::Push, {0, -1}, {-1, 0});
const Action Action::PushWS("Push(W,S)", ActionType::Push, {0, -1}, {1, 0});

const Action Action::PullNN("Pull(N,N)", ActionType::Pull, {-1, 0}, {-1, 0});
const Action Action::PullNE("Pull(N,E)", ActionType::Pull, {-1, 0}, {0, 1});
const Action Action::PullNW("Pull(N,W)", ActionType::Pull, {-1, 0}, {0, -1});
const Action Action::PullSS("Pull(S,S)", ActionType::Pull, {1, 0}, {1, 0});
const Action Action::PullSE("Pull(S,E)", ActionType::Pull, {1, 0}, {0, 1});
const Action Action::PullSW("Pull(S,W)", ActionType::Pull, {1, 0}, {0, -1});
const Action Action::PullEE("Pull(E,E)", ActionType::Pull, {0, 1}, {0, 1});
const Action Action::PullEN("Pull(E,N)", ActionType::Pull, {0, 1}, {-1, 0});
const Action Action::PullES("Pull(E,S)", ActionType::Pull, {0, 1}, {1, 0});
const Action Action::PullWW("Pull(W,W)", ActionType::Pull, {0, -1}, {0, -1});
const Action Action::PullWN("Pull(W,N)", ActionType::Pull, {0, -1}, {-1, 0});
const Action Action::PullWS("Pull(W,S)", ActionType::Pull, {0, -1}, {1, 0});

const std::array<const Action *, 29> &Action::allValues() {
    static const std::array<const Action *, 29> allActions = {
        &Action::NoOp,   &Action::MoveN,  &Action::MoveS,  &Action::MoveE,  &Action::MoveW,  &Action::PushNN,
        &Action::PushNE, &Action::PushNW, &Action::PushSS, &Action::PushSE, &Action::PushSW, &Action::PushEE,
        &Action::PushEN, &Action::PushES, &Action::PushWW, &Action::PushWN, &Action::PushWS, &Action::PullNN,
        &Action::PullNE, &Action::PullNW, &Action::PullSS, &Action::PullSE, &Action::PullSW, &Action::PullEE,
        &Action::PullEN, &Action::PullES, &Action::PullWW, &Action::PullWN, &Action::PullWS};
    return allActions;
}

std::string formatJointAction(const std::vector<const Action *> &joint_action, bool with_bubble) {
    static const size_t max_action_string_length = 20;

    std::string result;
    result.reserve(joint_action.size() * max_action_string_length);

    for (const auto &action : joint_action) {
        result += action->name + (with_bubble ? "@" + action->name : "");
        result += "|";
    }
    result.pop_back();
    return result;
}