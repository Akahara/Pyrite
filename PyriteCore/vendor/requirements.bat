#!/bin/bash

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "Git is not installed. Please install Git and try again."
    exit 1
fi

# Set the path where vcpkg will be cloned
VCPKG_PATH="$PWD/vcpkg"

# Check if the vcpkg path exists, clone if necessary
if [ ! -d "$VCPKG_PATH" ]; then
    echo "Cloning vcpkg repository from GitHub..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_PATH"
    if [ $? -ne 0 ]; then
        echo "Failed to clone vcpkg repository. Please check your internet connection and try again."
        exit 1
    fi
fi

# Navigate to vcpkg path and bootstrap vcpkg
cd "$VCPKG_PATH" || exit 1
echo "Setting up vcpkg..."
./bootstrap-vcpkg.sh
if [ $? -ne 0 ]; then
    echo "Failed to bootstrap vcpkg. Please check for errors and try again."
    exit 1
fi

# Install Assimp using vcpkg
echo "Installing Assimp..."
./vcpkg install assimp
if [ $? -ne 0 ]; then
    echo "Failed to install Assimp. Please check for errors and try again."
    exit 1
fi

echo "Assimp installed successfully."