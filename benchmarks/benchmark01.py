#!/usr/bin/env python3
import benchmark_api
import os

def main():
    print("Python script CWD:", os.getcwd())

    # Define your parameters
    levels = [
        "SAD1.lvl",
        "MAPF00.lvl"
    ]
    strategies = [
        "-bfs",
        "-dfs",
        "-astar"
    ]
    heuristics = [
        "boxgoalcount"
    ]

    # Decide where to save logs (folder under benchmarks/)
    logs_dir = "logs"

    # Call the API
    benchmark_api.run_benchmarks(
        levels=levels,
        strategies=strategies,
        heuristics=heuristics,
        logs_dir=logs_dir,
        memory_size="8g",           # e.g., 8g for 8GB
        step_limit=150,             # e.g., limit of 150 steps
        compile_before_run=True,    # compile the client before running
    )

if __name__ == "__main__":
    main()
