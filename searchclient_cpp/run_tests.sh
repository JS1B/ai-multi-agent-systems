#!/bin/bash

# ./run_tests.sh
# ./run_tests.sh debug verbose
# ./run_tests.sh debug list
# ./run_tests.sh release

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Default to debug build
BUILD_TYPE=${1:-debug}
BUILD_DIR="$SCRIPT_DIR/build_$BUILD_TYPE"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory $BUILD_DIR does not exist. Please build the project first."
    exit 1
fi

# Run tests with different options based on arguments
if [ "$2" == "verbose" ]; then
    # Run with verbose output
    cd "$BUILD_DIR" && ctest --output-on-failure -V
elif [ "$2" == "list" ]; then
    # List all tests
    cd "$BUILD_DIR" && ctest -N
else
    # Run tests normally
    cd "$BUILD_DIR" && ctest --output-on-failure
fi