# AI and Multi-Agent Systems

## Dependencies

This project uses `git submodules` for the FlameGraph library.

```bash
git submodule update --init
```

### Useful commands

Bash
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

If nix + devenv installed, for reproducible envinronment

```bash
devenv shell
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