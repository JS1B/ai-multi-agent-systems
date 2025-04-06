#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Create release build directory if it doesn't exist
if [ ! -d "$SCRIPT_DIR/build_release" ]; then
    mkdir "$SCRIPT_DIR/build_release"
fi

# Enter release build directory
cd "$SCRIPT_DIR/build_release"

# Configure CMake for release build
cmake -DCMAKE_BUILD_TYPE=Release "$SCRIPT_DIR"

# Build the project
cmake --build .

# Return to original directory
cd "$SCRIPT_DIR" 