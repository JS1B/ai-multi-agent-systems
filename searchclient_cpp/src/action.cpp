#include "action.hpp"

// Define all static members - cannot be initialized in the header file if .hpp is included in multiple files
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