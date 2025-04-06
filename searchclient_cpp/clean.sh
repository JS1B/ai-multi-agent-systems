#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Function to clean a build directory
clean_build() {
    if [ -d "$1" ]; then
        echo "Cleaning $1 directory..."
        rm -rf "$1"
    else
        echo "$1 directory does not exist."
    fi
}

# Clean both build directories
clean_build "$SCRIPT_DIR/build_debug"
clean_build "$SCRIPT_DIR/build_release"

echo "Cleaning complete." 