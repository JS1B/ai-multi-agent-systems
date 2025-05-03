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
If nix + devenv installed, for reproducible envinronment
```bash
devenv shell
```
To profile do this with this command (change what you run when needed):
```bash
java -jar server.jar -c "cargo flamegraph --manifest-path=searchclient_rust/Cargo.toml" -l "levels/warmup/MAsimple1.lvl" -g
```

## C/C++ Development Environment

This repository provides a consistent C/C++ development environment using Docker or Nix.

### Quick Start (Docker)

No need to install any development tools locally! Just use Docker:

```bash
# Pull the pre-built Docker image
docker pull ghcr.io/OWNER/REPO/cpp-dev-environment:latest

# Run the development environment with your code mounted
docker run -it --rm -v $(pwd):/workspace ghcr.io/OWNER/REPO/cpp-dev-environment:latest
```

Replace `OWNER/REPO` with your GitHub username and repository name.

### What's Included

The development environment contains:
- Clang tools
- CMake
- Conan
- Cppcheck
- Doxygen
- Google Test (gtest)
- LCOV
- vcpkg
- And more

### Building Locally

If you want to build the project:

```bash
# Inside the Docker container
cd /workspace
cmake -B build
cmake --build build
```

### For Nix Users

If you're familiar with Nix, you can also use:

```bash
nix develop
```