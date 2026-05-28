# Data-Oriented Core

The current model stores agents, box bulks, and states as nested objects and vectors. That is convenient for iteration, but expensive for search. The redesign should use compact, explicit state data and keep domain metadata separate from mutable search state.

## Design goals

- Make state copies small and predictable.
- Make hashing and equality fast.
- Make occupancy queries O(1).
- Avoid per-child heap allocation where possible.
- Keep ownership obvious through arenas, handles, and value buffers.
- Support several planner experiments without rewriting parsing or server output.

## Proposed split

### Static domain

Immutable after parsing:

- grid width and height
- wall bitset
- cell indexing helpers
- agent symbols and colors
- box symbols and colors
- goal cells by entity type
- precomputed distances or reachability maps
- level decomposition data

### Dynamic state

Mutable per search node:

- agent positions as dense arrays
- box positions as dense arrays
- occupancy arrays or sparse stamps
- parent node handle
- action taken from parent
- g cost
- cached hash
- cached heuristic

## SoA sketch

Instead of:

- `vector<Agent>`
- `vector<BoxBulk>`
- each object holding positions/goals/symbols

Use:

- `agent_cell[agent_id]`
- `agent_color[agent_id]`
- `agent_goal_range[agent_id]`
- `box_cell[box_id]`
- `box_symbol[box_id]`
- `box_color[box_id]`
- `box_goal_range[box_id]`
- `cell_agent[cell]` or stamped occupancy
- `cell_box[cell]` or stamped occupancy

Symbols and colors belong to static metadata. Positions belong to dynamic state.

## State identity

Use a compact canonical state key:

- agent cells in fixed agent order
- box cells in fixed box identity order
- optional timestep for temporal constraints

For indistinguishable boxes of the same symbol/color, choose one strategy:

1. Give every parsed box a stable identity. This is faster and simpler.
2. Canonicalize boxes within a symbol group by sorted cell order. This can reduce duplicates but costs sorting or incremental maintenance.

For raw performance, start with stable identity and measure.

## Memory ownership

Use explicit stores:

- `NodeArena` owns all generated nodes for one low-level search.
- `CtNodeStore` owns high-level nodes.
- frontiers store node handles, not owning raw pointers.
- parent links are handles into the owning arena.
- plans are reconstructed from handles only when needed.

This keeps leaks from controlling the design without making every state a separate smart pointer allocation.

## Occupancy strategy

Prefer fixed-size arrays indexed by cell:

- `int16_t agent_at[cell_count]`
- `int16_t box_at[cell_count]`
- `uint32_t stamp[cell_count]` for temporary per-expansion marks

For very small levels, dense arrays are simpler and likely faster than hash maps. For large sparse levels, keep the same API and swap implementation later if profiling proves it necessary.

## Incremental migration path

1. Introduce `LevelStaticData` and `StateView` beside current classes.
2. Add conversion from current `Level` to the new static/dynamic representation.
3. Port hashing, occupancy, and action application to the new representation.
4. Keep existing server output and parser unchanged.
5. Port low-level expansion to use handles and arrays.
6. Delete the old object-heavy state path after benchmarks match or improve.

## Performance bets

- Fewer allocations during expansion.
- Faster `getBoxAt` and `isCellFree`.
- Cheaper equality and hashing.
- Smaller parent chains.
- Easier SIMD/cache-friendly scans later.
