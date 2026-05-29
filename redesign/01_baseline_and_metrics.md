# Baseline and Metrics

Raw performance work needs a stable scoreboard before any redesign starts. The code should optimize for solve rate first, then runtime, generated states, and memory.

## Benchmark scope

Use two suites:

1. `warmup_smoke`: small levels that should solve quickly and catch regressions.
2. `comp_probe`: a curated subset of competition levels, including unsolved or currently hard cases.

Do not require all `comp` levels to solve at first. A failed level is still useful if the run records timeout, generated state count, peak memory, and failure reason.

## Primary metrics

| Metric | Why it matters |
| --- | --- |
| solved | Primary objective. |
| timeout | Distinguishes hard failures from planner bugs. |
| wall_time_ms | User-visible throughput. |
| generated_states | Measures branching pressure. |
| expanded_states | Measures real search work. |
| peak_rss_mb | Captures state explosion and ownership problems. |
| actions_used | Useful quality signal, but secondary to solved/time. |
| failure_reason | Separates timeout, memory cap, empty frontier, parser error, and invalid action. |

## Benchmark command shape

The Phase 0 benchmark runner supports:

- `--suite warmup_smoke`
- `--suite comp_probe`
- `--timeout-s N`
- `--runs N`
- `--jobs N`
- `--planner NAME`
- `--output-dir DIR`

Build mode stays outside the runner for now: build `searchclient_cpp/searchclient` with `make` or `make RELEASE=true` before running the suite. The `--planner` argument is a report label unless `--pass-planner-arg` is explicitly enabled.

## Baseline table format

Each row should be one `(level, planner, run)` result:

| level | planner | solved | timeout | wall_time_ms | generated | expanded | peak_rss_mb | actions | failure_reason |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

The summary should include:

- solved count by suite
- median wall time on solved levels
- timeout count
- memory-cap count
- top 10 levels by generated states
- top 10 levels by peak RSS

## Implementation notes

- Add a tiny server smoke test before running full benchmarks.
- Prefer release builds for benchmark numbers, but keep debug smoke tests for correctness.
- Store benchmark metadata: git SHA, compiler, flags, planner name, feature flags, level suite, timeout, and CPU count.
- Use single-run numbers for quick iteration; use multi-run medians only when comparing competing designs.
