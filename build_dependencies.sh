#!/bin/bash

# VSG Dependencies Build Script for Linux/macOS
# This script fetches and builds the pinned VSG library dependencies needed for dop-gui

set -e  # Exit on any error

echo "================================================================="
echo "VSG Dependencies Build Script for Linux/macOS"
echo "================================================================="
echo "This script will build pinned VulkanSceneGraph, vsgXchange, and vsgImGui revisions"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
if ! command_exists cmake; then
    echo "Error: CMake is not installed or not in PATH"
    echo "Please install CMake (version 3.7 or later)"
    echo "  Ubuntu/Debian: sudo apt install cmake"
    echo "  macOS: brew install cmake"
    exit 1
fi

if ! command_exists git; then
    echo "Error: Git is not installed or not in PATH"
    echo "Please install Git"
    echo "  Ubuntu/Debian: sudo apt install git"
    echo "  macOS: brew install git"
    exit 1
fi

if ! command_exists make; then
    echo "Error: Make is not installed or not in PATH"
    echo "Please install build essentials"
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  macOS: xcode-select --install"
    exit 1
fi

# Determine number of parallel jobs
if command_exists nproc; then
    JOBS=$(nproc)
elif command_exists sysctl; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=4
fi

echo "Using $JOBS parallel jobs for building"
echo ""

VSG_REF="${VSG_REF:-7c56d802d4403df7f38293c4f3cfa5029945404c}"
VSGXCHANGE_REF="${VSGXCHANGE_REF:-2e96fd624d8786ae411762e3d54455e6b8ab5593}"
VSGIMGUI_REF="${VSGIMGUI_REF:-5014eeded201288d002448b1014bf88c19529e81}"

echo "Pinned dependency refs:"
echo "  VulkanSceneGraph: $VSG_REF"
echo "  vsgXchange:       $VSGXCHANGE_REF"
echo "  vsgImGui:         $VSGIMGUI_REF"
echo ""

# Go up one directory level to work alongside dop-gui
cd ..

# Create dependencies directory
echo "Creating dependencies directory..."
mkdir -p vsg_deps
cd vsg_deps

INSTALL_DIR="$(pwd)/install"
echo "Dependencies will be installed to: $INSTALL_DIR"
echo ""

# Function to clone repository if it doesn't exist
clone_if_needed() {
    local repo_name=$1
    local repo_url=$2
    
    if [ ! -d "$repo_name" ]; then
        echo "Cloning $repo_name..."
        git clone "$repo_url"
        if [ $? -ne 0 ]; then
            echo "Error: Failed to clone $repo_name"
            exit 1
        fi
    else
        echo "$repo_name already exists, skipping clone..."
    fi
}

checkout_ref() {
    local repo_name=$1
    local repo_ref=$2

    echo "Checking out $repo_name at $repo_ref..."
    cd "$repo_name"
    git fetch --tags --all
    git checkout "$repo_ref"
    cd ..
}

# Function to build and install a project
build_and_install() {
    local project_name=$1
    local cmake_args="${2:-}"
    
    echo ""
    echo "================================================================="
    echo "Building $project_name"
    echo "================================================================="
    
    cd "$project_name"
    mkdir -p build
    cd build
    
    echo "Configuring $project_name..."
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          $cmake_args \
          ..
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to configure $project_name"
        exit 1
    fi
    
    echo "Building $project_name..."
    make -j$JOBS
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build $project_name"
        exit 1
    fi
    
    echo "Installing $project_name..."
    make install
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to install $project_name"
        exit 1
    fi
    
    cd ../..
}

# Clone repositories
clone_if_needed "VulkanSceneGraph" "https://github.com/vsg-dev/VulkanSceneGraph.git"
clone_if_needed "vsgXchange" "https://github.com/vsg-dev/vsgXchange.git"
clone_if_needed "vsgImGui" "https://github.com/vsg-dev/vsgImGui.git"

# Pin repositories to known working refs for reproducible fresh installs
checkout_ref "VulkanSceneGraph" "$VSG_REF"
checkout_ref "vsgXchange" "$VSGXCHANGE_REF"
checkout_ref "vsgImGui" "$VSGIMGUI_REF"

# Build VulkanSceneGraph first (required by others)
build_and_install "VulkanSceneGraph"

# Build vsgXchange (depends on VSG)
build_and_install "vsgXchange" "-Dvsg_DIR=$INSTALL_DIR/lib/cmake/vsg"

# Build vsgImGui (depends on VSG)
build_and_install "vsgImGui" "-Dvsg_DIR=$INSTALL_DIR/lib/cmake/vsg"

echo ""
echo "================================================================="
echo "Dependencies Build Complete!"
echo "================================================================="
echo ""
echo "All VSG dependencies have been built and installed to: $INSTALL_DIR"
echo ""
echo "Directory structure:"
echo "vsg_deps/"
echo "  ├── VulkanSceneGraph/"
echo "  ├── vsgXchange/"
echo "  ├── vsgImGui/"
echo "  └── install/"
echo "      ├── bin/"
echo "      ├── lib/"
echo "      └── include/"
echo ""
echo "To build dop-gui, you can now use these dependencies by setting:"
echo "  -Dvsg_DIR=../vsg_deps/install/lib/cmake/vsg"
echo "  -DvsgXchange_DIR=../vsg_deps/install/lib/cmake/vsgXchange"
echo "  -DvsgImGui_DIR=../vsg_deps/install/lib/cmake/vsgImGui"
echo ""
echo "Example dop-gui build commands:"
echo "  cd dop-gui"
echo "  mkdir -p build && cd build"
echo "  cmake -DCMAKE_BUILD_TYPE=Release \\"
echo "        -Dvsg_DIR=../vsg_deps/install/lib/cmake/vsg \\"
echo "        -DvsgXchange_DIR=../vsg_deps/install/lib/cmake/vsgXchange \\"
echo "        -DvsgImGui_DIR=../vsg_deps/install/lib/cmake/vsgImGui \\"
echo "        .."
echo "  make -j$JOBS"
echo ""
