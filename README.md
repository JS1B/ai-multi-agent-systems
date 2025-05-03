# AI and Multi-Agent Systems

Presentation Link: [Course Introduction Slides](https://docs.google.com/presentation/d/181soPRH8DPw9JhZXqvjlQJYNIl39AJvE0wjCEwSXI_Y/edit?usp=sharing)

This repository provides development environments and code for the AI Multi-Agent Systems course, focusing primarily on the C++ search client, but also including resources for other languages.

## Setup Options for C++ Development

You can set up the C++ development environment using either Nix directly or Docker/Podman.

### 1. Using Nix Flakes (Recommended on Linux/macOS)

This is the preferred method if you have Nix installed with Flakes support enabled on your host machine.

1.  **Install Nix:** Follow the instructions at [https://nixos.org/download.html](https://nixos.org/download.html). Ensure you enable Flakes support during or after installation.
2.  **Enter the Environment:** Navigate to the project directory in your terminal and run:
    ```bash
    nix develop
    ```
    This command downloads all necessary tools specified in `flake.nix` (compilers, libraries, debuggers, etc.) and starts a shell session with these tools available in the `PATH`. The `shellHook` in `flake.nix` will also run, attempting to install additional tools like `flamegraph` via Cargo.

### 2. Using Docker or Podman (Recommended for Windows or Container Fans)

This method uses the provided `Dockerfile` to build a container image containing the complete Nix environment. It's useful if you prefer containerization or are running on an operating system where native Nix setup is more complex (like Windows).

1.  **Prerequisites:**
    *   Docker ([Install Docker](https://docs.docker.com/engine/install/)) or Podman ([Install Podman](https://podman.io/getting-started/installation)).
    *   Ensure a `flake.lock` file exists in the project directory. If not, run `nix flake lock` (requires Nix on the host) or proceed with the build (it might fetch dependencies without a lock, which is less reproducible).

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
    Start an interactive container session with your project code mounted into the container's `/workspace` directory:
    ```bash
    # Using Docker (replace $(pwd) with %cd% on Windows CMD, or ${PWD} on PowerShell)
    docker run -it --rm --privileged -v "$(pwd):/workspace" ai-mas-dev

    # Using Podman (replace $(pwd) as above; :Z is for SELinux systems)
    podman run -it --rm --privileged -v "$(pwd):/workspace:Z" ai-mas-dev
    ```
    *   `-v "$(pwd):/workspace"`: This mounts your current project directory (on the host) into the `/workspace` directory inside the container. This allows you to edit code using your preferred editor on your host machine, and compile/run it inside the container's environment.
    *   `--privileged`: This flag is **required**. The Dockerfile enables Nix sandboxing (`sandbox = true` in `/etc/nix/nix.conf`) within the container for better build reproducibility, and sandboxing requires elevated permissions. It's also often needed for performance profiling tools like `perf`.
    *   The container automatically starts the Nix development environment defined in `flake.nix` (via `CMD ["nix", "develop", "--command", "bash"]`). You'll land in a bash shell ready to work.

## Included Tools (via Nix Flake / Dockerfile)

The C++ environment provides:

*   **C/C++:** `clang`, `clang-tools`, `cmake`, `gtest`
*   **Rust:** `cargo`, `rustc` (primarily for `flamegraph`/`inferno`)
*   **Debugging:** `gdb`
*   **Profiling:** `perf`, `flamegraph`, `inferno` (some installed via `shellHook`)
*   **Code Quality:** `cppcheck`, `codespell`, `doxygen`, `lcov`
*   **Utilities:** `make`, `which`, `file`, `bash`

## Working with the C++ Search Client

Once inside the development environment (either via `nix develop` or the Docker/Podman container):

```bash
# Navigate to the C++ client directory
cd searchclient_cpp

# Build the client
make

# Run the client (example)
./searchclient bfs < ../levels/level0.lvl

# Run using the make target (reads level from ../levels/default.lvl)
# make run

# Clean build files
make clean
```

## Profiling the C++ Search Client

Profiling helps identify performance bottlenecks. The environment includes `perf` and `flamegraph`.

### 1. Using the Nix App Defined in `flake.nix` (Recommended)

The simplest way is to use the `profile-cpp` app, which is defined directly within the `flake.nix` file using `pkgs.writeShellScript`. Run it from the project root (works in both `nix develop` and the container):

```bash
# Profile with a specific level file (default algorithm: bfs)
nix run .#profile-cpp -- levels/custom/custom00.lvl

# Profile with a specific level file and algorithm
nix run .#profile-cpp -- levels/custom/custom00.lvl astar
```

This Nix app automatically:
*   Builds the `searchclient` if needed.
*   Runs `flamegraph` with optimal settings.
*   Saves the result to `searchclient_cpp/flamegraph.svg`.
*   Cleans up temporary files.

### 2. Manual Profiling

For more control, run the tools manually inside the `searchclient_cpp` directory:

```bash
# Create an input file (example)
cat ../levels/custom/custom00.lvl > input.txt

# Option A: Using flamegraph directly
flamegraph --freq 999 --flamechart --min-width 0.1 -o flamegraph.svg -- ./searchclient bfs < input.txt

# Option B: Using perf and inferno separately
perf record -F 99 -e cpu-clock -g -- ./searchclient bfs < input.txt
perf script | inferno-flamegraph > flamegraph.svg

# Clean up perf data if needed
# rm perf.data*
```

### Troubleshooting Perf Permissions

If `perf` or `flamegraph` gives permission errors:

1.  **Inside Docker/Podman:** Ensure you launched the container with the `--privileged` flag.
2.  **On Host Linux (Non-NixOS):** You might need to adjust system settings:
    ```bash
    sudo sysctl kernel.perf_event_paranoid=-1
    sudo sysctl kernel.kptr_restrict=0
    # Optional, increase buffer size if needed
    # sudo sysctl kernel.perf_event_mlock_kb=65535
    ```
    These changes might reset on reboot. Consult your distribution's documentation for making them permanent.
3.  **On Host NixOS:** Performance event permissions might be managed differently. Consult NixOS documentation or consider using the container method.

## Java Client Commands (Legacy/Reference)

If working with the Java client:

Bash:
```bash
javac searchclient/*.java && java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180
```

Powershell:
```powershell
javac searchclient/*.java ; java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180
```

## Notes

*   The `shellHook` in `flake.nix` (and thus the Docker container startup) attempts to install `flamegraph` and `inferno` using `cargo install`. This requires network access when the environment is first entered/started.
*   Using `perf` effectively often requires access to kernel symbols, which might be limited inside containers depending on the host setup, even with `--privileged`.