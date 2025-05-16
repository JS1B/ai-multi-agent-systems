# Searchclient cpp

It describes how to use the included C++ searchclient with the server that is contained in server.jar.

The search client requires a C++17 compatible compiler (g++).

All the following commands assume the working directory is the one this readme is located in.

You can read about the server options using the -h argument:

```
java -jar ../server.jar -h
```

Compiling the searchclient:
```
make
```

By default, this compiles with optimizations (`RELEASE=true`). For a debug build with debug symbols and no optimizations (recommended for Valgrind or GDB), run:
```
make clean && make RELEASE=false
```
To switch back to a release build:
```
make clean && make RELEASE=true
```

Starting the server using the searchclient:

```
java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient" -g -s 150 -t 180
```

The searchclient uses the BFS strategy by default. Use arguments like `-s dfs`, `-s astar`, etc., passed to the executable, to set alternative search strategies (after you implement them). For instance, to use DFS on the same level as above (assuming `./searchclient` accepts `-s <strategy>`):

```
java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient -s dfs" -g -s 150 -t 180
```
Consult the searchclient's command-line argument parsing in `src/searchclient.cpp` for available options.

## Debugging and Profiling

### Debugging with Valgrind

To detect memory errors (e.g., leaks, invalid memory access), you can run the searchclient with Valgrind. It's highly recommended to compile in debug mode first:

```
make clean && make RELEASE=false
```

Then, use the Makefile target:

```
make valgrind_run
```
This runs `./searchclient` with a default strategy and level (see `FLAMEGRAPH_STRATEGY` and `LEVEL_FILE` in the `Makefile`, currently defaults to `bfs` and `../levels/warmup/MAPF02C.lvl` respectively). Valgrind output is saved to `valgrind-out.txt`.

To use a different strategy or level for the Valgrind run, you can override the Makefile variables:
```
make valgrind_run VALGRIND_STRATEGY=astar VALGRIND_LEVEL_FILE=../levels/custom.lvl
```

### Profiling with Flame Graphs

To identify CPU performance bottlenecks, you can generate a flame graph using `perf` and the FlameGraph toolkit.

1.  **Ensure `perf` is installed and you have permissions.**
    You might need `sudo` or to adjust `/proc/sys/kernel/perf_event_paranoid` (e.g., `echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid`).
2.  **Ensure the FlameGraph toolkit is available.**
    The Makefile expects it at `../lib/FlameGraph`. Clone it there if needed: `git clone https://github.com/brendangregg/FlameGraph ../lib/FlameGraph`.
3.  **Compile your program (preferably release build for profiling performance):**
    ```
    make
    ```
4.  **Run the Makefile target:**
    ```
    make flamegraph
    ```
This generates `cpu_flamegraph.svg` using default level and strategy (see `FLAMEGRAPH_STRATEGY` and `LEVEL_FILE` in `Makefile`). Open the SVG in a browser.