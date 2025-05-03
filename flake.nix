{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      # Development shell with all the tools needed
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            # Override stdenv in order to change compiler:
            # stdenv = pkgs.clangStdenv;
          }
          {
            packages = with pkgs; [
              # Core development tools
              clang-tools
              cmake
              gtest
              cargo
              rustc
              
              # Debugging and profiling
              gdb
              linuxPackages.perf
              
              # Code quality and documentation
              codespell
              cppcheck
              doxygen
              lcov
              
              # Basic utilities
              gnumake
              which
              file
              bash
            ] ++ (if system == "aarch64-darwin" then [ ] else [ gdb ]);
            
            # Set up environment variables
            shellHook = ''
              # Install flamegraph using Cargo
              echo "Installing flamegraph with Cargo..."
              cargo install flamegraph || echo "Failed to install flamegraph, might be already installed or network issue"
              
              # Install inferno tools for flamegraph generation
              echo "Installing inferno tools..."
              cargo install inferno || echo "Failed to install inferno, might be already installed or network issue"
              
              # Add cargo bin directory to PATH
              export PATH="$HOME/.cargo/bin:$PATH"
              
              echo "C/C++ Development Environment Loaded"
              echo "Available tools: clang, cmake, gtest, gdb, etc."
              echo "Profiling tools: perf, flamegraph, inferno-flamegraph"
              echo ""
              echo "For successful profiling, use:"
              echo "  nix run .#profile-cpp -- LEVEL_FILE SEARCH_ALGORITHM"
              echo "Example:"
              echo "  nix run .#profile-cpp -- levels/custom/custom00.lvl bfs"
              echo ""
              echo "Or run flamegraph directly:"
              echo "  flamegraph --freq 999 --flamechart --min-width 0.1 -o flamegraph.svg -- ./searchclient_cpp/searchclient bfs < input.txt"
            '';
          };
      });

      # Profiling app for C++ searchclient
      apps = forEachSupportedSystem ({ pkgs }: {
        profile-cpp = {
          type = "app";
          program = toString (pkgs.writeShellScript "profile-cpp.sh" ''
            #!/usr/bin/env bash
            set -euo pipefail

            # Check if arguments are provided
            if [ $# -lt 1 ]; then
              echo "Usage: nix run .#profile-cpp -- LEVEL_FILE [SEARCH_ALGORITHM]"
              echo "Example: nix run .#profile-cpp -- levels/custom/custom00.lvl bfs"
              exit 1
            fi

            # Set defaults and parse arguments
            LEVEL_FILE="$1"
            ALGORITHM="bfs"  # default
            if [ $# -ge 2 ]; then
              ALGORITHM="$2"
            fi

            # Check if level file exists
            if [ ! -f "$LEVEL_FILE" ]; then
              echo "Error: Level file '$LEVEL_FILE' not found"
              exit 1
            fi

            # Check if searchclient exists
            if [ ! -f "searchclient_cpp/searchclient" ]; then
              echo "Building searchclient..."
              make -C searchclient_cpp
            fi

            echo "Profiling searchclient with level: $LEVEL_FILE and algorithm: $ALGORITHM"
            
            # Create input file
            cat "$LEVEL_FILE" > input.txt
            
            # Run flamegraph with optimal settings
            cd searchclient_cpp
            export PATH="$HOME/.cargo/bin:$PATH"
            flamegraph --freq 999 --flamechart --min-width 0.1 -o flamegraph.svg -- ./searchclient $ALGORITHM < ../input.txt
            
            # Check if flamegraph was generated successfully
            if [ -f "flamegraph.svg" ]; then
              echo "Profiling complete. Flamegraph saved to searchclient_cpp/flamegraph.svg"
              
              # Clean up temporary files
              echo "Cleaning up temporary files..."
              rm -f ../input.txt
              rm -f perf.data perf.data.old
              
              # Remove intermediate flamegraph files but NOT the final SVG
              rm -f flamegraph.folded
              rm -f flamegraph.stacks
              rm -f *.perf-folded
              
              echo "Cleanup complete. Flamegraph saved at: $(pwd)/flamegraph.svg"
            else
              echo "Error: Flamegraph generation failed"
              exit 1
            fi
          '');
        };
      });
    };
}