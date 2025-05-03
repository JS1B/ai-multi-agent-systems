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

# AI Multi-Agent Systems - C++ Development Environment

This repository provides a development environment for the C++ search client used in the AI Multi-Agent Systems course. It utilizes Nix Flakes for reproducibility and dependency management.

## Setup Options

You can set up the development environment using either Nix directly or Docker/Podman.

### 1. Using Nix Flakes (Recommended)

This is the preferred method if you have Nix installed with Flakes support enabled.

1.  **Install Nix:** Follow the instructions at [https://nixos.org/download.html](https://nixos.org/download.html). Ensure you enable Flakes support.
2.  **Enter the Environment:** Navigate to the project directory in your terminal and run:
    ```bash
    nix develop
    ```
    This command downloads all necessary tools (compilers, libraries, debuggers, etc.) specified in `flake.nix` and starts a shell session with these tools available in the `PATH`. The `shellHook` in `flake.nix` will also run, potentially installing additional tools like `flamegraph` via Cargo.

### 2. Using Docker or Podman

This method uses a Dockerfile to build a container image containing the Nix environment. It's useful if you prefer containerization or don't have Nix installed directly on your host machine.

1.  **Prerequisites:**
    *   Docker ([Install Docker](https://docs.docker.com/engine/install/)) or Podman ([Install Podman](https://podman.io/getting-started/installation)).
    *   Ensure your Nix installation (if any on the host) has generated a `flake.lock` file. If not, run `nix flake lock` in the project directory.

2.  **Build the Image:**
    Navigate to the project directory and run:
    ```bash
    # Using Docker
    docker build -t ai-mas-dev .

    # Using Podman
    podman build -t ai-mas-dev .
    ```
    (Replace `ai-mas-dev` with your preferred image tag).

3.  **Run the Container:**
    Start an interactive container session:
    ```bash
    # Using Docker
    docker run -it --rm --privileged ai-mas-dev

    # Using Podman
    podman run -it --rm --privileged ai-mas-dev
    ```
    *   `--privileged`: This flag is **required** because the Dockerfile enables Nix sandboxing (`sandbox = true` in `/etc/nix/nix.conf`) within the container for better build reproducibility. Sandboxing needs elevated permissions to function correctly.
    *   The container will automatically start the Nix development environment defined in `flake.nix` (via `CMD ["nix", "develop", "--command", "bash"]`).

## Included Tools

The environment provides:

*   **C/C++:** `clang`, `clang-tools`, `cmake`, `gtest`
*   **Rust:** `cargo`, `rustc` (for `flamegraph`/`inferno`)
*   **Debugging:** `gdb`
*   **Profiling:** `perf`, `flamegraph`, `inferno` (installed via `shellHook`)
*   **Code Quality:** `cppcheck`, `codespell`, `doxygen`, `lcov`
*   **Utilities:** `make`, `which`, `file`, `bash`

## Building the C++ Search Client

Once inside the development environment (either via `nix develop` or the Docker/Podman container):

```bash
ncd searchclient_cpp
make
```

## Running the C++ Search Client

```bash
./searchclient <algorithm> < input.txt
# Example:
./searchclient bfs < ../levels/level0.lvl
```

## Profiling the C++ Search Client

A Nix app is provided for easy profiling with `flamegraph`.

**From the project root directory (outside the C++ client directory):**

```bash
nix run .#profile-cpp -- <level_file> [search_algorithm]
```

**Examples:**

```bash
# Profile with BFS on custom00.lvl
nix run .#profile-cpp -- levels/custom/custom00.lvl bfs

# Profile with A* on level1.lvl (default algorithm is bfs if not specified)
nix run .#profile-cpp -- levels/level1.lvl astar 
```

This command will:
1.  Build the `searchclient` if it doesn't exist.
2.  Run `flamegraph` with recommended settings.
3.  Save the output SVG to `searchclient_cpp/flamegraph.svg`.
4.  Clean up temporary files.

Alternatively, you can run `flamegraph` manually inside the `searchclient_cpp` directory:

```bash
flamegraph --freq 999 --flamechart --min-width 0.1 -o flamegraph.svg -- ./searchclient <algorithm> < ../levels/<level_file>
```

## Notes

*   The `shellHook` in `flake.nix` attempts to install `flamegraph` and `inferno` using `cargo install`. This requires network access when the environment is first entered (or when the Docker image is built/run, depending on caching).
*   Using `perf` inside a Docker container might have limitations depending on the host system configuration and container runtime capabilities.