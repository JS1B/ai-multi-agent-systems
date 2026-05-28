# Redesign Roadmap

This directory captures a performance-first redesign plan for the MAvis C++ client. The goal is raw solve throughput on `levels/warmup` and `levels/comp`, not optimality guarantees.

## North star

- Maximize solved levels under the server timeout.
- Keep memory ownership explicit enough that leaks and lifetime bugs do not shape algorithm choices.
- Prefer data layouts and planner APIs that make profiling, pruning, and replacement easy.
- Treat the current CBS implementation as a useful prototype, not as the architecture to preserve.

## Current constraints

- The active client is a color-grouped CBS planner: agents and boxes are grouped by color, low-level A* plans each group, and high-level CBS resolves conflicts across groups.
- The main scalability limit is joint action branching: `29^agent_count_in_group` action tuples are generated before filtering.
- Tests and benchmarks are not yet a trustworthy performance signal. Some tests target an older `Level` API, and benchmark strategies are passed on the command line but ignored by the client.
- The report already identifies two important symptoms: rising memory use and rare missed box-agent conflicts.

## Step-by-step roadmap

| Phase | Target | What changes | Output |
| --- | --- | --- | --- |
| 0 | Measurement first | Build a small repeatable benchmark suite over warmup and a curated comp subset; record solve rate, timeout count, actions, generated states, expanded states, peak RSS, and wall time. | A baseline table and repeatable command. |
| 1 | Simple fixes | Repair stale tests, make `make test` reliable, fix benchmark strategy drift, add smoke tests through `server.jar`, and patch obvious conflict misses. | Trustworthy red/green checks. |
| 2 | Hot-path cleanup | Fix explored/frontier lookups, remove unnecessary plan copies, cache heuristic values, index boxes by cell, and prune joint actions before allocating child states. | Lower generated states and memory churn without changing the planner shape. |
| 3 | Ownership and API cleanup | Replace raw ownership in CBS, frontier, graph search, and state pools with explicit owners; introduce stable plan, state, constraint, and metrics types. | Clear architecture boundaries and fewer lifetime questions. |
| 4 | Data-oriented state core | Move from object-heavy AoS state copies to compact SoA arrays for agent positions, box positions, occupancy, constraints, and parent/action metadata. | Faster state expansion, cheaper hashing, and lower memory per node. |
| 5 | Planner redesign | Replace brute-force color-group CBS with a performance-oriented planner: targeted successor generation, dynamic grouping, bounded/suboptimal search, reservation tables, decomposition, and explicit box trajectory handling. | Higher warmup/comp solve rate. |
| 6 | Tuning loop | Profile, benchmark, and delete losing ideas quickly. Keep interfaces stable so planner experiments can be swapped without rewriting parsing/output. | Iterative performance improvement. |

## Suggested document flow

1. `01_baseline_and_metrics.md` defines the measurement plan.
2. `02_fast_fixes.md` lists low-risk repairs and optimizations.
3. `03_data_oriented_core.md` sketches the SoA architecture.
4. `04_planner_redesign.md` proposes the non-optimal, throughput-oriented planner path.
5. `05_decision_questions.md` lists the decisions to settle before implementation.

## Initial recommendation

Start with phases 0 through 2. They are small enough to validate quickly, and they will make later algorithm work measurable. Then move to phase 4 before phase 5: a clean SoA state core gives the redesign a fast substrate and avoids baking new algorithms into the current object model.
