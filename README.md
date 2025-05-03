# AI and Multi-Agent Systems

Prezka mordy
[link](https://docs.google.com/presentation/d/181soPRH8DPw9JhZXqvjlQJYNIl39AJvE0wjCEwSXI_Y/edit?usp=sharing)

### Useful commands

Bash
```bash
javac searchclient/*.java && java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```

Powershell
```powershell
javac searchclient/*.java ; java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```

## C/C++ Development Environment

This repository provides a consistent C/C++ development environment using Nix via Docker.

### Quick Start (Docker)

```bash
# Build the Docker image
docker build -t cpp-nix-dev .

# Run the development environment with your code mounted
# --privileged flag is needed for performance profiling
docker run -it --privileged -v $(pwd):/code cpp-nix-dev
```

For Podman users (recommended):
```bash
# Build with Podman
podman build -t cpp-nix-dev .

# Run with Podman (note the :Z flag for SELinux contexts)
podman run -it --privileged -v $(pwd):/code:Z cpp-nix-dev
```

### What's Included

The development environment contains:
- Clang tools
- CMake
- GDB
- Google Test (gtest)
- Flamegraph & perf (profiling)
- And other tools defined in the flake.nix

### Building and Running C++ Code

Once inside the development environment, use the Makefile in the searchclient_cpp directory:

```bash
# Build the C++ client
make -C searchclient_cpp

# Run the executable
make -C searchclient_cpp run

# Clean build files
make -C searchclient_cpp clean
```

### Profiling with Flamegraph

The environment includes performance profiling tools. There are two ways to use them:

#### 1. Using the Nix App (Recommended)

The simplest way to profile your code is to use the built-in `profile-cpp` app:

```bash
# Profile with a specific level file (default algorithm: bfs)
nix run .#profile-cpp -- levels/custom/custom00.lvl

# Profile with a specific level file and algorithm
nix run .#profile-cpp -- levels/custom/custom00.lvl bfs

# Profile with a different algorithm
nix run .#profile-cpp -- levels/MAPF00.lvl greedy
```

This automatically:
- Builds the searchclient if needed
- Sets up proper profiling parameters
- Creates an optimized flamegraph
- Cleans up temporary files
- Saves the result to `searchclient_cpp/flamegraph.svg`

#### 2. Manual Profiling

For more control, you can run the profiling tools directly:

```bash
# Change to searchclient directory
cd searchclient_cpp

# Create input file
cat ../levels/custom/custom00.lvl > input.txt

# Option 1: Using flamegraph directly
flamegraph --freq 999 --flamechart --min-width 0.1 -o flamegraph.svg -- ./searchclient bfs < input.txt

# Option 2: Using perf and inferno separately
perf record -F 99 -e cpu-clock -g -- ./searchclient bfs < input.txt
perf script | inferno-flamegraph > flamegraph.svg
```

#### Troubleshooting Perf Permissions

If you're getting permission errors with perf:

1. For NixOS users: Import the module from this flake
2. For Docker/Podman: Make sure to run with `--privileged`
3. For other Linux: Set these sysctl parameters:
   ```bash
   sudo sysctl kernel.perf_event_paranoid=-1
   sudo sysctl kernel.kptr_restrict=0
   sudo sysctl kernel.perf_event_mlock_kb=65535
   ```

### For Nix Users

If you have Nix installed locally, you can also use:

```bash
# Enable flakes if not already enabled
nix --extra-experimental-features "nix-command flakes" develop

# Or if flakes are already enabled
nix develop
```