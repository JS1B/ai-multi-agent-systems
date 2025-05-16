# AI and Multi-Agent Systems

## Dependencies

This project uses `git submodules` for the FlameGraph library. Make sure they are initialized:

```bash
git submodule update --init --recursive
```

## C++ Client (`searchclient_cpp`)

The C++ version of the search client is located in the `searchclient_cpp` directory. It uses a Makefile for building and managing tasks.

### Building the Client

To build the C++ client, navigate to its directory and use `make`:

```bash
cd searchclient_cpp
make
```
Or, from the project root:
```bash
make -C searchclient_cpp
```
This will compile the sources and create an executable named `searchclient` in the `searchclient_cpp` directory.

### Running the Client

To run the compiled client:

```bash
cd searchclient_cpp
make run
```
This typically expects input from stdin. To run with a specific level file:
```bash
cd searchclient_cpp
cat ../levels/warmup/MAPF00.lvl | ./searchclient
```
Or, from the project root:
```bash
cat levels/warmup/MAPF00.lvl | ./searchclient_cpp/searchclient
```

The client supports various command-line arguments to control its behavior:
*   `-s <strategy>`: Specifies the search strategy. Available strategies include `bfs`, `dfs`, `astar`, `wastar`, `greedy`.
*   `-hc <heuristic_calculator>`: Specifies the heuristic calculator.
    *   Available calculators: `zero`, `mds` (Manhattan Distance for agents), `sic` (Sum of Individual Costs for agents to their goals), `bgm` (Box-Goal Manhattan Distance), `maxof` (uses a combination of mds, sic, bgm).
    *   For Saturated Cost Partitioning (SCP), use the format: `scp:<sub_heuristic1>,<sub_heuristic2>,...`
        *   Example: `-hc scp:sic,pdb_all` or `-hc scp:pdb_all,sic,mds`
        *   Available sub-heuristics for SCP: `sic`, `pdb_all` (all single-box pattern databases), `bgm`, `mds`, `zero`.
        *   The order of sub-heuristics in the SCP string matters for the partitioning.

Example with strategy and specific heuristic:
```bash
cat levels/warmup/MAPF00.lvl | ./searchclient_cpp/searchclient -s astar -hc sic
```
Example with SCP:
```bash
cat levels/warmup/MAPF00.lvl | ./searchclient_cpp/searchclient -s astar -hc scp:sic,pdb_all
```

You can also use it with the server:
```bash
java -jar server.jar -c "./searchclient_cpp/searchclient -s astar -hc scp:sic,pdb_all" -l "levels/warmup/MAPF00.lvl" -g
```

### Benchmarking

The `searchclient_cpp` project includes a Python-based benchmarking script located at `benchmarks/run_benchmarks.py`. This script automates running the C++ client against multiple levels and strategy/heuristic combinations.

**Prerequisites:**
*   Python 3.x
*   The `rich` Python library: `pip install rich`

**Configuration:**
Benchmark cases are defined in `benchmarks/benchmarks.json`. This file specifies:
*   `levels_dir`: The directory containing level files.
*   `output_dir`: Where to save benchmark result JSON files.
*   `cases`: A list of test cases, each specifying:
    *   `input`: The relative path to the level file (from `levels_dir`).
    *   `strategies`: A list of command-line argument strings to pass to the C++ client (e.g., `"-s astar -hc sic"`, `"-s wastar -hc scp:pdb_all,sic"`).
*   `timeout_seconds_per_case` (optional): Timeout in seconds for each individual run (defaults to 600s).

**Running Benchmarks:**
The recommended way to run the benchmarks is using the `Makefile` target from within the `searchclient_cpp` directory. This ensures the C++ client is compiled first.

```bash
cd searchclient_cpp
make run_benchmark
```

This command will:
*   Build the `searchclient` executable if it's outdated.
*   Change to the benchmark directory (assumed to be `../benchmarks` relative to `searchclient_cpp`).
*   Execute the `run_benchmarks.py` script (e.g., using `uv run run_benchmarks.py` as per the current Makefile).

The script will then:
*   Read `benchmarks.json` (expected to be in the directory where `run_benchmarks.py` resides).
*   Run the C++ client in parallel for each configured case.
*   Display a live progress table using `rich`.
*   Save results (including metrics parsed from the C++ client and timing information) to a timestamped JSON file in the `output_dir` (specified in `benchmarks.json`).
*   You can press 'q' during the run to request a graceful shutdown (pending tasks will be cancelled, running tasks will complete or timeout).

If you need to run the script manually (e.g., for development or if not using `make`):
1. Ensure the C++ client (`searchclient_cpp/searchclient`) is already compiled.
2. Navigate to the directory containing `run_benchmarks.py` (e.g., `cd benchmarks` from the project root).
3. Run the script: `python3 run_benchmarks.py` (or `uv run run_benchmarks.py`).
   Make sure `benchmarks.json` is in this directory, and adjust `CPP_EXECUTABLE_PATH` in `run_benchmarks.py` if necessary.

**Example `benchmarks.json` structure:**
```json
{
    "levels_dir": "../levels",
    "output_dir": "benchmark_results",
    "timeout_seconds_per_case": 300,
    "cases": [
        {
            "input": "warmup/MAPF00.lvl",
            "strategies": [
                "-s astar -hc zero",
                "-s astar -hc scp:pdb_all,sic",
                "-s wastar -hc scp:sic,pdb_all"
            ]
        },
        // ... more cases
    ]
}
```

### Cleaning Build Files

To remove compiled object files and the executable:

```bash
cd searchclient_cpp
make clean
```
Or, from the project root:
```bash
make -C searchclient_cpp clean
```

### Profiling with Flamegraph

The `Makefile` in `searchclient_cpp` includes a convenient target for generating flame graphs using `perf` and the `FlameGraph` scripts.

**Prerequisites:**
1.  Ensure you have `perf` and the FlameGraph scripts (from the `lib/FlameGraph` submodule) accessible in your PATH.
2.  `perf` often requires specific permissions. If you encounter errors:
    *   Inside Docker/Podman (if you decide to use it later): Run the container with `--privileged`.
    *   On Linux: You might need to adjust `kernel.perf_event_paranoid`. For example:
        ```bash
        sudo sysctl kernel.perf_event_paranoid=-1
        sudo sysctl kernel.kptr_restrict=0 
        ```
        (The `Makefile` itself provides a note on this.)

**Generating the Flamegraph:**

```bash
cd searchclient_cpp
make flamegraph
```
This will:
1.  Run `perf record` on the `searchclient` with a default level and strategy (defined in the Makefile: `LEVEL_FILE = ../levels/warmup/MAPF02C.lvl`, `FLAMEGRAPH_STRATEGY = bfs`).
2.  Process the data using `perf script` and `stackcollapse-perf.pl`.
3.  Generate `cpu_flamegraph.svg` using `flamegraph.pl`.
4.  Clean up intermediate files.

You can customize the `LEVEL_FILE` and `FLAMEGRAPH_STRATEGY` variables in the `Makefile` or by passing them on the command line:
```bash
cd searchclient_cpp
make flamegraph FLAMEGRAPH_STRATEGY=astar LEVEL_FILE=../levels/custom/my_level.lvl
```

### Debugging with GDB

To debug the C++ client, you'll need GDB installed and available in your system's PATH. You can debug using GDB directly or via a code editor's debugging interface.

The following `launch.json` configuration can be used with VS Code for debugging. Place it in `.vscode/launch.json` in your project root.

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug C++ SearchClient (MAPF00.lvl)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/searchclient_cpp/searchclient",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/searchclient_cpp",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Set Disassembly Flavor to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                },
                {
                    "description": "Redirect stdin from level file for the 'run' command",
                    "text": "set args < ../levels/warmup/MAPF00.lvl",
                    "ignoreFailures": false
                }
            ]
        }
    ]
}
```
To use this:
1. Ensure the client is built (e.g., `make -C searchclient_cpp`).
2. Open the `searchclient_cpp` source files in VS Code.
3. Set breakpoints.
4. Go to the "Run and Debug" panel and select "Debug C++ SearchClient (MAPF00.lvl)" and start debugging.

---
*Previous Java/Rust specific commands have been removed or incorporated if general. Add them back if other parts of the project still use them.*

```bash
javac searchclient/*.java && java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```

```bash
java -jar server.jar -c "./searchclient_cpp/searchclient" -l "levels/warmup/MAPF00.lvl" -g
```

```bash
cat levels/warmup/MAPF00.lvl | ./searchclient_cpp/searchclient
```

Powershell
```powershell
javac searchclient/*.java ; java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```

To profile do this with this command (change what you run when needed):

```bash
java -jar server.jar -c "cargo flamegraph --manifest-path=searchclient_rust/Cargo.toml" -l "levels/warmup/MAsimple1.lvl" -g
```

### Debugging

To debug the searchclient_cpp, you can use the following launch.json:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug SearchClient (MAPF00.lvl)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/searchclient_cpp/searchclient",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceFolder}/searchclient_cpp",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Set Disassembly Flavor to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                },
                {
                    "description": "Redirect stdin from level file for the 'run' command",
                    "text": "set args < ../levels/warmup/MAPF00.lvl",
                    "ignoreFailures": false
                }
            ]
        }
    ]
}
```