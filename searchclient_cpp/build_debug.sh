#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Create debug build directory if it doesn't exist
if [ ! -d "$SCRIPT_DIR/build_debug" ]; then
    mkdir "$SCRIPT_DIR/build_debug"
fi

# Enter debug build directory
cd "$SCRIPT_DIR/build_debug"

# Configure CMake for debug build
cmake -DCMAKE_BUILD_TYPE=Debug "$SCRIPT_DIR"

# Build the project
cmake --build .

# Return to original directory
cd "$SCRIPT_DIR" 