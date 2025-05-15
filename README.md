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

You can also use it with the server:
```bash
java -jar server.jar -c "./searchclient_cpp/searchclient" -l "levels/warmup/MAPF00.lvl" -g
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