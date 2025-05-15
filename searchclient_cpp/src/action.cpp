#include "action.hpp"

#include "point2d.hpp"

// Point2D(col_x, row_y)
// N: row_y decreases (0, -1)
// S: row_y increases (0, 1)
// E: col_x increases (1, 0)
// W: col_x decreases (-1, 0)

Action::Action(const std::string &name, const ActionType type, const Point2D agentDelta, const Point2D boxDelta)
    : name(name), type(type), agentDelta(agentDelta), boxDelta(boxDelta) {}

const Action Action::NoOp("NoOp", ActionType::NoOp, {0, 0}, {0, 0});

// Move actions - Corrected Deltas
const Action Action::MoveN("Move(N)", ActionType::Move, {0, -1}, {0, 0});
const Action Action::MoveS("Move(S)", ActionType::Move, {0, 1}, {0, 0});
const Action Action::MoveE("Move(E)", ActionType::Move, {1, 0}, {0, 0});
const Action Action::MoveW("Move(W)", ActionType::Move, {-1, 0}, {0, 0});

// Push actions - Corrected Deltas
const Action Action::PushNN("Push(N,N)", ActionType::Push, {0, -1}, {0, -1}); // Agent North, Box North
const Action Action::PushNE("Push(N,E)", ActionType::Push, {0, -1}, {1, 0});  // Agent North, Box East
const Action Action::PushNW("Push(N,W)", ActionType::Push, {0, -1}, {-1, 0}); // Agent North, Box West
const Action Action::PushSS("Push(S,S)", ActionType::Push, {0, 1}, {0, 1});   // Agent South, Box South
const Action Action::PushSE("Push(S,E)", ActionType::Push, {0, 1}, {1, 0});   // Agent South, Box East
const Action Action::PushSW("Push(S,W)", ActionType::Push, {0, 1}, {-1, 0});  // Agent South, Box West
const Action Action::PushEE("Push(E,E)", ActionType::Push, {1, 0}, {1, 0});   // Agent East,  Box East
const Action Action::PushEN("Push(E,N)", ActionType::Push, {1, 0}, {0, -1});  // Agent East,  Box North
const Action Action::PushES("Push(E,S)", ActionType::Push, {1, 0}, {0, 1});   // Agent East,  Box South
const Action Action::PushWW("Push(W,W)", ActionType::Push, {-1, 0}, {-1, 0}); // Agent West,  Box West
const Action Action::PushWN("Push(W,N)", ActionType::Push, {-1, 0}, {0, -1});  // Agent West,  Box North
const Action Action::PushWS("Push(W,S)", ActionType::Push, {-1, 0}, {0, 1});   // Agent West,  Box South

// Pull actions - Corrected Deltas
// Agent moves to new cell, box moves to agent's old cell.
// Example: Pull(N,N) - Agent moves North, Box was North of Agent (relative to agent's NEW pos), Box also moves North (ends up where agent started)
// Agent new_pos = agent old_pos + agent_delta
// Box old_pos = agent new_pos - box_delta (this is where box must be relative to agent's NEW pos for action to be named Pull(A,B))
// No, simpler: Box old_pos = agent old_pos - agent_delta (box is initially *behind* agent relative to agent's move direction)
// Agent moves from P_old to P_new. Box moves from (P_old - agent_delta) to P_old.
// agentDelta is agent's move. boxDelta is where box was relative to agent P_OLD.
// No, the existing interpretation: agentDelta is where agent moves. boxDelta is where box IS relative to NEW agent pos.
// Let's re-verify Pull logic based on common interpretation: Agent moves agentDelta. Box is at agent_start_pos - action.agentDelta and moves to agent_start_pos.
// The provided `action.boxDelta` for Pull in `isApplicable` and `result` is actually `action.agentDelta` in many student codes.
// The Pull actions might need more careful thought on their deltas depending on game logic for Pull.
// For now, assuming agentDelta = agent's move, boxDelta = where box moves TO relative to its start.
// A common Pull: Agent moves P_old -> P_new. Box at B_old (behind agent) moves B_old -> P_old.
// So, agentDelta is (P_new - P_old). BoxDelta is (P_old - B_old).
// If Agent pulls North (moves North), box was South of agent, box also moves North.
// AgentDelta = (0,-1) (Agent N). Box starts at AgentOldPos + (0,1). Box ends at AgentOldPos.
// For Pull(X,Y): X is direction agent moves. Y is direction box moves.
// The current (original) Pull deltas look like agentDelta is agent move, boxDelta is box move.
// This is unconventional for Pull usually. Let's assume the names are literal: Agent pulls in direction X, box follows in direction Y.

const Action Action::PullNN("Pull(N,N)", ActionType::Pull, {0, -1}, {0, -1}); // Agent North, Box North
const Action Action::PullNE("Pull(N,E)", ActionType::Pull, {0, -1}, {1, 0});  // Agent North, Box East
const Action Action::PullNW("Pull(N,W)", ActionType::Pull, {0, -1}, {-1, 0}); // Agent North, Box West
const Action Action::PullSS("Pull(S,S)", ActionType::Pull, {0, 1}, {0, 1});   // Agent South, Box South
const Action Action::PullSE("Pull(S,E)", ActionType::Pull, {0, 1}, {1, 0});   // Agent South, Box East
const Action Action::PullSW("Pull(S,W)", ActionType::Pull, {0, 1}, {-1, 0});  // Agent South, Box West
const Action Action::PullEE("Pull(E,E)", ActionType::Pull, {1, 0}, {1, 0});   // Agent East,  Box East
const Action Action::PullEN("Pull(E,N)", ActionType::Pull, {1, 0}, {0, -1});  // Agent East,  Box North
const Action Action::PullES("Pull(E,S)", ActionType::Pull, {1, 0}, {0, 1});   // Agent East,  Box South
const Action Action::PullWW("Pull(W,W)", ActionType::Pull, {-1, 0}, {-1, 0}); // Agent West,  Box West
const Action Action::PullWN("Pull(W,N)", ActionType::Pull, {-1, 0}, {0, -1});  // Agent West,  Box North
const Action Action::PullWS("Pull(W,S)", ActionType::Pull, {-1, 0}, {0, 1});   // Agent West,  Box South


const std::array<const Action *, 29> &Action::allValues() {
    static const std::array<const Action *, 29> allActions = {
        &Action::NoOp,   &Action::MoveN,  &Action::MoveS,  &Action::MoveE,  &Action::MoveW,  &Action::PushNN,
        &Action::PushNE, &Action::PushNW, &Action::PushSS, &Action::PushSE, &Action::PushSW, &Action::PushEE,
        &Action::PushEN, &Action::PushES, &Action::PushWW, &Action::PushWN, &Action::PushWS, &Action::PullNN,
        &Action::PullNE, &Action::PullNW, &Action::PullSS, &Action::PullSE, &Action::PullSW, &Action::PullEE,
        &Action::PullEN, &Action::PullES, &Action::PullWW, &Action::PullWN, &Action::PullWS};
    return allActions;
}