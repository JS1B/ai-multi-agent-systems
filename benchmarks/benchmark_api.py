#!/usr/bin/env python3
import subprocess
import os

def run_benchmarks(
    levels,
    strategies,
    heuristics,
    logs_dir="logs",
    memory_size="8g",
    step_limit=150,
    server_jar_path="../server.jar",
    searchclient_dir="../searchclient_java/searchclient",
    levels_dir="../levels",
    compile_before_run=True,
    gui=False
):
    """
    Runs benchmarks for the given levels, strategies, and heuristics.
    
    Parameters:
    -----------
    levels : list of str
        Level filenames (e.g., ["SAD1.lvl", "MAthomasAppartment.lvl"]).
    strategies : list of str
        Strategies (e.g., ["-bfs", "-dfs", "-astar"]).
    heuristics : list of str
        Heuristics (e.g., ["boxgoalcount", "myHeuristic"]).
    logs_dir : str
        Directory (relative to benchmarks/) where log files will be saved.
    memory_size : str
        JVM Xmx memory setting (e.g., "4g", "8g").
    step_limit : int
        The -s parameter for the server (number of steps).
    server_jar_path : str
        Path to server.jar relative to benchmarks/ folder.
    searchclient_dir : str
        Path to the folder containing *.java for the search client.
    levels_dir : str
        Path to the folder containing level files.
    compile_before_run : bool
        Whether to compile the Java code before running.
    gui : bool
        Whether to run the server with GUI (-g).
    """

    # 1) Optionally compile the Java search client
    if compile_before_run:
        compile_command = f"javac {searchclient_dir}/*.java"
        print(f"Compiling searchclient with: {compile_command}")
        subprocess.run(compile_command, shell=True, check=True)

    # 2) Ensure the logs directory exists (relative to benchmarks/)
    os.makedirs(logs_dir, exist_ok=True)

    # 3) Build the GUI flag for the server
    gui_flag = "-g" if gui else ""

    # 4) Iterate over all combinations of levels, strategies, heuristics
    for level in levels:
        for strategy in strategies:
            for heuristic in heuristics:
                # Construct the server command
                #
                # Example:
                #   java -jar ../server.jar 
                #       -l ../levels/SAD1.lvl
                #       -c "java -Xmx8g searchclient_java.SearchClient -heur boxgoalcount -astar"
                #       -g -s 150
                command = (
                    f"java -jar {server_jar_path} "
                    f"-l {levels_dir}/{level} "
                    f"-c \"java -cp searchclient_java -Xmx{memory_size} searchclient_java.SearchClient -heur {heuristic} {strategy}\" "
                    f"{gui_flag} -s {step_limit}"
                )

                # Print info to stdout
                print(f"\n--- Running Level={level}, Strategy={strategy}, Heuristic={heuristic} ---")
                print("Command:", command)

                # Generate a unique log file name
                level_name = level.replace(".lvl", "")
                strategy_name = strategy.lstrip("-")  # remove leading dash for cleanliness
                log_filename = f"{logs_dir}/{level_name}_{strategy_name}_{heuristic}.log"

                # Run and capture logs
                with open(log_filename, "w") as log_file:
                    subprocess.run(command, shell=True, stdout=log_file, stderr=log_file, check=False)

                print(f"Logs saved to: {log_filename}")

    print("\nBenchmarking complete!")
