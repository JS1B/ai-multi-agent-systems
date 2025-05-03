{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });

      # Define the core packages needed for both devShell and Docker image
      commonPackages = pkgs: with pkgs; [
        # Core development tools
        clang-tools
        cmake
        gtest
        # cargo # Removed, using Nix package for flamegraph/inferno
        # rustc # Removed
        
        # Debugging and profiling
        # gdb # Removed for size
        linuxPackages.perf # Note: perf might have limited functionality inside Docker
        flamegraph # Added Nix package
        inferno    # Added Nix package
        
        # Code quality and documentation (Removed for size)
        # codespell
        # cppcheck
        # doxygen
        # lcov
        
        # Basic utilities
        gnumake
        which
        file
        bash # Ensure bash is available for the CMD
      ]; # Darwin check for gdb removed as gdb is removed

      # Define the shell hook content
      devShellHook = pkgs: ''
        # Ensure .cargo/bin is in PATH (still useful if user manually installs rust tools)
        # export PATH="$HOME/.cargo/bin:$PATH"

        # No longer installing via cargo, flamegraph/inferno provided by Nix
        echo "Flamegraph and inferno tools provided by Nix."
        
        echo ""
        echo "----------------------------------------"
        echo " Nix C/C++ Development Environment"
        echo "----------------------------------------"
        echo " Tools: clang, cmake, gtest, make, etc."
        echo " Profile: perf, flamegraph, inferno"
        echo "----------------------------------------"
        echo " Use 'nix run .#profile-cpp -- <level> <algo>' for profiling."
        echo " Example: nix run .#profile-cpp -- levels/custom/custom00.lvl bfs"
        echo "----------------------------------------"
      '';

    in
    {
      # Development shell with all the tools needed (includes debug/quality tools)
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            # Override stdenv in order to change compiler:
            # stdenv = pkgs.clangStdenv;
          }
          {
            # Dev shell can still include the extra tools for local development
            packages = with pkgs; commonPackages pkgs ++ [
              gdb # Add gdb back for local dev shell
              codespell
              cppcheck
              doxygen
              lcov
              # Optionally add cargo/rustc here if needed for local rust dev
              # cargo
              # rustc 
            ];
            shellHook = devShellHook pkgs;
          };
      });

      # Packages output including the Docker image definition (minimal tools)
      packages = forEachSupportedSystem ({ pkgs }: {
        dockerImage = pkgs.dockerTools.buildImage {
          name = "ai-multi-agent-dev-env";
          tag = "latest";
          
          # Include necessary packages (minimal set)
          contents = commonPackages pkgs;

          # Run the shell hook equivalent on container start
          # We create a script that sources the environment and runs the hook, then bash
          config = {
            Cmd = [ 
              "${pkgs.bash}/bin/bash" 
              "-c" 
              # This script simulates entering the dev shell:
              # 1. Sources profile scripts to set up paths etc. (like nix develop would)
              # 2. Executes the shellHook content
              # 3. Starts an interactive bash session
              ''
                # Source Nix environment setup (adjust path if needed, but this is typical)
                if [ -e /etc/profile.d/nix.sh ]; then . /etc/profile.d/nix.sh; fi
                
                # Execute the shell hook manually
                ${devShellHook pkgs}
                
                # Start interactive bash shell
                exec ${pkgs.bash}/bin/bash
              '' 
            ];
            WorkingDir = "/workspace"; # Set a working directory inside the container
          };

          # Copy the project files into the image's /workspace directory
          runAsRoot = ''
            ${pkgs.dockerTools.usrBinEnv}
            # Create a workspace directory and copy project files
            mkdir /workspace
            cp -rT . /workspace # Copy everything from the build context
            
            # Ensure the workspace is owned by the default nix-user (or root if runAsRoot)
            # Adjust user/group as needed if you configure a specific user
            chown -R $(id -u):$(id -g) /workspace 
            
            # Optional: Pre-build the C++ client if desired
            # echo "Pre-building searchclient..."
            # cd /workspace/searchclient_cpp && make
          '';

        };
      });

      # Profiling app for C++ searchclient (Remains unchanged, uses tools from environment)
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
            SEARCHCLIENT_PATH="searchclient_cpp/searchclient"
            if [ ! -x "$SEARCHCLIENT_PATH" ]; then
              echo "Building searchclient..."
              if ! make -C searchclient_cpp; then
                echo "Error: Failed to build searchclient"
                exit 1
              fi
              if [ ! -x "$SEARCHCLIENT_PATH" ]; then # Check again after build
                 echo "Error: searchclient executable not found after build"
                 exit 1
              fi
            fi


            echo "Profiling searchclient with level: $LEVEL_FILE and algorithm: $ALGORITHM"
            
            # Create input file in a temporary location
            INPUT_FILE=$(mktemp)
            trap 'rm -f "$INPUT_FILE"' EXIT # Ensure cleanup
            cat "$LEVEL_FILE" > "$INPUT_FILE"
            
            # Ensure cargo bin is in PATH for flamegraph (Commented out - no longer needed by default)
            # export PATH="$HOME/.cargo/bin:$PATH"
            
            # Check if flamegraph is installed (Should always be, as it's in commonPackages)
            if ! command -v flamegraph &> /dev/null; then
              echo "Error: flamegraph not found. This shouldn't happen as it's included via Nix."
              exit 1
            fi

            # Run flamegraph with optimal settings, relative to workspace root
            FLAMEGRAPH_OUTPUT="searchclient_cpp/flamegraph.svg"
            echo "Running flamegraph..."
            if ! flamegraph --freq 999 --flamechart --min-width 0.1 -o "$FLAMEGRAPH_OUTPUT" -- ./"$SEARCHCLIENT_PATH" $ALGORITHM < "$INPUT_FILE"; then
              echo "Error: flamegraph command failed"
              # Attempt to clean up perf files even on failure
              rm -f perf.data perf.data.old flamegraph.folded flamegraph.stacks *.perf-folded
              exit 1
            fi
            
            # Check if flamegraph was generated successfully
            if [ -f "$FLAMEGRAPH_OUTPUT" ]; then
              echo "Profiling complete. Flamegraph saved to $FLAMEGRAPH_OUTPUT"
              
              # Clean up temporary and intermediate files
              echo "Cleaning up temporary files..."
              rm -f perf.data perf.data.old # These might be in the CWD where nix run was executed
              rm -f searchclient_cpp/perf.data searchclient_cpp/perf.data.old # Or inside the cpp dir
              rm -f flamegraph.folded flamegraph.stacks *.perf-folded # Also likely in CWD
              rm -f searchclient_cpp/flamegraph.folded searchclient_cpp/flamegraph.stacks searchclient_cpp/*.perf-folded # Or inside cpp dir

              echo "Cleanup complete. Flamegraph saved at: $(pwd)/$FLAMEGRAPH_OUTPUT"
            else
              echo "Error: Flamegraph generation failed - SVG file not found."
              exit 1
            fi
          '');
        };
      });
    };
}