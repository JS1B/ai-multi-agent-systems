# Benchmarks

`run_benchmarks.py` is the Phase 0 redesign scoreboard. It runs the built C++ client through `server.jar`, records per-level outcomes, and writes a JSON report under `benchmarks/output/`.

## Quick smoke run

From the repository root:

```bash
cd searchclient_cpp
make RELEASE=true -j
cd ../benchmarks
python3 run_benchmarks.py --suite warmup_smoke --jobs 1
```

## Probe competition levels

```bash
cd benchmarks
python3 run_benchmarks.py --suite comp_probe --jobs 1 --timeout-s 10
```

Many `comp_probe` cases may timeout or fail today. That is expected; the failure reason is part of the baseline signal.

## Output schema

Each JSON report includes:

- metadata: suite, client path, server path, runs, jobs, planner label
- summary: solved count, timeout count, median solved wall time, median server search time, median harness overhead, top generated/expanded states, top max frontier, top peak RSS, throughput outliers, and failure reasons
- cases: one row per `(level, run)` with solved status, timeout status, total wall time, server search time, harness overhead, generated states, expanded states, final/max frontier, final/max peak RSS, status sample count, generated/sec, expanded/sec, generated/expanded, actions/sec, action count, and failure reason

The `--planner` value is currently only a label. Use `--pass-planner-arg` only after the client actually implements planner selection.
