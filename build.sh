#!/bin/bash

# Enhanced IL2CPP Dumper Build Script

set -e  # Exit immediately if a command exits with a non-zero status

echo "Enhanced IL2CPP Dumper Build Script"
echo "====================================="

# Check if cmake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed"
    exit 1
fi

# Check if a C++ compiler is installed
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "Error: No C++ compiler found (g++ or clang++)"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo "Configuring project with CMake..."
cmake ..

echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build completed successfully!"
echo "The executable is located at: \$(pwd)/il2cpp-dumper"
echo ""
echo "To run the tool:"
echo "  ./il2cpp-dumper <path_to_global-metadata.dat> [optional_path_to_libil2cpp.so]"
echo ""
