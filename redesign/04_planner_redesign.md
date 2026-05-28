# Planner Redesign

The redesign does not need optimality guarantees. That frees the planner to use bounded, greedy, prioritized, and decomposition-based techniques that solve more levels under fixed time.

## Current bottleneck

The current planner groups agents by color, then generates the Cartesian product of all 29 actions for every agent in that group. This is the wrong scaling shape for raw performance. The redesign should avoid full joint enumeration unless a small group truly needs it.

## Candidate planner direction

Use a hybrid planner:

1. Preprocess the level.
2. Assign boxes/goals/agents greedily or with min-cost matching.
3. Plan agents with a reservation table.
4. Escalate only local conflicts to small coupled groups.
5. Replan failed windows rather than restarting all CBS search.

This favors solve rate over proof of optimality.

## Stage A: targeted successors

Replace full joint tuples with targeted action generation:

- generate legal actions per agent
- prioritize actions that move toward assigned work
- include NoOp only when useful
- reject local collisions before creating child nodes
- branch one active agent or one small group at a time

This can be implemented before replacing the whole high-level planner.

## Stage B: dynamic grouping

Start with agents independent or in very small groups. Merge only when repeated conflicts prove coupling is necessary.

Possible grouping signals:

- same narrow corridor
- same box or goal region
- repeated reservation-table collisions
- same connected component after static obstacle preprocessing
- one agent must move a blocker for another

This is the opposite of the current color-first merge. Color remains a constraint for box manipulation, not necessarily the search grouping key.

## Stage C: reservation-table planning

Maintain reservations over `(cell, time)` and optionally `(edge, time)`:

- agent occupancy
- box occupancy
- pushed/pulled box paths
- temporary blocked cells for corridors

Plan agents/box tasks in priority order. If a lower-priority plan fails, use local repair or reprioritization.

This will not be complete, but it is fast and can solve many levels quickly.

## Stage D: box-task planning

Treat box movement as the central task:

1. choose an agent for a box-goal pair
2. move the agent near the box
3. generate a push/pull route for the box
4. reserve agent and box trajectories
5. repair conflicts locally

Heuristics should include:

- agent-to-box distance
- box-to-goal distance
- wall-aware distances
- corridor penalties
- number of blocking boxes
- estimated pushes/pulls

## Stage E: bounded best-first search

Where CBS remains useful, make it intentionally suboptimal:

- weighted A*
- ECBS-style focal search
- conflict count tie-breakers
- time-budgeted search
- restart with different priorities
- stop when first valid solution is found

Quality can be improved after solve rate improves.

## Stage F: decomposition

Use level structure:

- connected components
- rooms and corridors
- articulation points
- goal regions
- boxes that never matter for active goals

Solve independent regions separately when possible. For dependent regions, solve the hardest bottleneck first and reserve its corridor windows.

## Stage G: fallback strategies

Keep several planners behind one interface:

- greedy prioritized planner
- dynamic-group repair planner
- weighted CBS fallback
- current CBS compatibility planner

The benchmark runner should compare planners by solve rate and timeout count, not by theoretical guarantees.

## Rejection criteria

Delete or demote a redesign idea if it:

- lowers warmup solve rate
- increases median solved-level runtime without improving comp solve count
- requires large API coupling to one planner
- makes benchmark output less comparable
- cannot explain failures with metrics
