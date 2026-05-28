# Fast Fixes and Local Optimizations

These tasks keep the current architecture mostly intact. They are useful because they improve signal, reduce obvious waste, and make later redesign work easier to judge.

## P0: restore correctness signal

1. Rewrite stale tests around the current `Level`, `StaticLevel`, `LowLevelState`, and `CBS` APIs.
2. Delete placeholder tests that only print success.
3. Make the `make test` loop fail immediately when a test executable fails.
4. Add one integration test that builds the client and solves a tiny warmup level through `server.jar`.
5. Add a benchmark smoke command that runs one known-solvable warmup level.

## P1: remove benchmark drift

1. Either wire command-line planner selection into `searchclient` or remove `strategy` from benchmark configs.
2. Make the benchmark output schema match the client/server metrics.
3. Treat unknown baselines and `999` placeholders as `unsolved`, not as real comparison data.
4. Add a small checked-in benchmark suite definition instead of requiring a local-only `benchmarks.json`.

## P2: patch obvious correctness bugs

1. Fix box-agent conflict attribution when a moving agent collides with a stationary box.
2. Fail loudly in debug builds when conflict detection cannot attribute a box conflict.
3. Add an empty-plan guard in plan merging.
4. Validate same-group joint actions after all individual applicability checks:
   - no two agents end in the same cell
   - no illegal swaps
   - no two agents manipulate the same box
   - no box-box collisions after the joint action

## P3: local hot-path optimizations

1. Use content hash lookup for explored states when constraints are empty.
2. Make frontier membership content-based rather than pointer-based.
3. Cache heuristic values per generated state.
4. Index boxes by cell for `getBoxAt`, `moveBox`, and conflict simulation.
5. Replace repeated full plan copies with indexed access and NoOp fallback.
6. Precompute constraints by timestep for each low-level replan.
7. Record box movers while simulating a timestep instead of rescanning actions for every conflict.

## P4: low-risk memory cleanup

1. Make CT node ownership explicit.
2. Ensure popped CT nodes are freed after expansion.
3. Ensure graph search clears explored states between replans.
4. Keep parent chains alive only for states that can still produce a plan.
5. Add peak memory metrics before changing allocator strategy.

These fixes are not the final architecture. Their value is that they make the current planner less noisy and provide a fair baseline for the redesign.
