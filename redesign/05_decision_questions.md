# Decision Questions

These questions should be answered before implementation starts. They define the first concrete plan.

## Benchmark questions

1. Which warmup levels form the mandatory smoke suite?
2. Which comp levels should form the first probe suite?
3. What timeout should be used for quick iteration?
4. Is solve rate the only ranking metric, or should action count break ties?
5. Should benchmark output be committed for selected runs, or only the suite definitions?

## Architecture questions

1. Should the SoA state core be introduced beside the current classes first, or replace them directly?
2. Should boxes get stable identities even when boxes of the same symbol are interchangeable?
3. Should the planner store full states, deltas, or parent handles plus action records?
4. Should static level preprocessing become mandatory before every planner run?
5. How much legacy compatibility is worth keeping once the benchmark suite exists?

## Planner questions

1. Should the first redesign target be targeted successors inside current CBS, or a separate prioritized planner?
2. Should color be used only for box compatibility, not for default search grouping?
3. Should failed local plans trigger dynamic group merging, priority reshuffling, or both?
4. What is the first acceptable fallback when the greedy planner fails?
5. Should the current CBS remain as a compatibility baseline while new planners are developed?

## Performance questions

1. What is the maximum acceptable memory use per level?
2. Should release builds keep `-march=native`, or should benchmark builds be portable and reproducible?
3. Is parallel benchmark execution acceptable, or should timing comparisons run single-job?
4. Should profiling focus first on generated state count, wall time, or peak RSS?
5. Is a custom arena allocator acceptable once ownership is handle-based?

## Recommended answers for the first milestone

1. Start with benchmark and test repair, because planner redesign without a scoreboard will be hard to judge.
2. Introduce the SoA core beside current classes, not as a direct replacement.
3. Give boxes stable identities at first for speed and simplicity.
4. Build targeted successors inside the current planner before a full new planner.
5. Keep current CBS as a benchmark baseline until the new planner solves more warmup levels.
