# AI and Multi-Agent Systems

## Running

Build the searchclient

```bash
# build
cd searchclient_cpp
make RELEASE=true -j
```

Single level

```bash
# run
./searchclient < levels/**/*.lvl # Select one

# run using server
java -jar server.jar -c ./searchclient -l ../levels/**/*.lvl # Select one 
# -g to get visualisation
# -s 150 to set speed per joint action
# -t 180 to set timeout
# -o to get output in replayable format
```

Multi-level

```bash
# run
java -jar server.jar -c ./searchclient -l ../levels/** # Select a directory
```

Or using a benchmarking script

```bash
cd benchmarks

pip install -r requirements.txt

python run_benchmarks.py
```

## Development

This project uses `git submodules` for the FlameGraph library.

```bash
git submodule update --init
```

Building

```bash
# build
cd searchclient_cpp
make -j # for debug
make flamegraph # for flamegraph generation - change the level file in the Makefile
```

### Debugging

To debug the searchclient_cpp, you can use the following launch.json:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug SearchClient (*.lvl)",
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
