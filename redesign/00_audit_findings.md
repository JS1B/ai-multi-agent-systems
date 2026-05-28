# Audit Findings

This file summarizes the code audit that motivates the roadmap.

## Working shape

- `searchclient_cpp/src/searchclient.cpp` reads one level, constructs `CBS`, solves, and streams actions to `server.jar`.
- `CBS` groups agents and boxes by color, then plans each color group as one low-level problem.
- `Graphsearch` runs best-first search over `LowLevelState`.
- `LowLevelState` stores agents, box bulks, parent pointer, and actions, then expands by enumerating joint action permutations.

## Main risks

| Area | Finding | Why it matters |
| --- | --- | --- |
| Tests | `test_level.cpp` and `test_state.cpp` target older APIs; `test_foo.cpp` is a placeholder. | Performance work needs a reliable red/green signal. |
| Benchmarks | Benchmark strategies are passed to the executable, but the executable ignores `argv`. | Comparisons across strategies are currently misleading. |
| Branching | Joint action generation creates `29^n` action tuples for a group. | This dominates runtime and memory for multi-agent color groups. |
| Conflict detection | Box conflicts can be skipped when no responsible moving agent is found. | The planner can accept invalid plans or fail to resolve real conflicts. |
| Ownership | CBS and graph search use raw pointers for nodes, states, frontiers, and searches. | Lifetime is unclear, and leaks can hide algorithmic performance issues. |
| State layout | Agents and boxes are object-heavy AoS structures copied into many search states. | State expansion, hashing, and occupancy checks are more expensive than needed. |
| Constraints | Constraints are only `(cell, time)` and apply to all agents in a group. | Replanning can over-constrain same-color groups. |
| Heuristic | The current heuristic is Manhattan-heavy and uses the first agent for box distance. | It gives weak guidance on box-heavy levels. |

## High-value source hotspots

- `searchclient_cpp/include/low_level_state.hpp`: state layout, occupancy checks, joint expansion, action application.
- `searchclient_cpp/src/cbs.cpp`: high-level CBS loop, CT node lifecycle, plan merging, conflict detection.
- `searchclient_cpp/include/graphsearch.hpp`: low-level loop, duplicate detection, constraint filtering.
- `searchclient_cpp/include/heuristic.hpp`: heuristic quality and repeated per-comparison work.
- `benchmarks/run_benchmarks.py`: benchmark contract and strategy wiring.
- `searchclient_cpp/tests/`: stale test signal.

## Design conclusion

The fastest path is not to polish optimal CBS. The faster path is:

1. measure the current solver honestly
2. remove obvious waste and broken signals
3. build a compact SoA state core
4. swap in non-optimal planners that prioritize solve rate
5. keep only the ideas that improve warmup and comp results
