#!/bin/bash

set -e

echo "=== Pome Installer ==="

# Check for root
if [ "$EUID" -ne 0 ]; then 
  echo "Please run as root (sudo ./install.sh)"
  exit 1
fi

BUILD_DIR="build"

echo "[1/4] Building Pome..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..

echo "[2/4] Installing Executable and Library..."
cp "$BUILD_DIR/pome" /usr/local/bin/
chmod +x /usr/local/bin/pome

# Install library
# Check if libpome.so exists (Linux) or dylib (Mac)
if [ -f "$BUILD_DIR/libpome.so" ]; then
    cp "$BUILD_DIR/libpome.so" /usr/local/lib/
    chmod 755 /usr/local/lib/libpome.so
    # Update ldconfig cache
    if command -v ldconfig &> /dev/null; then
        ldconfig
    fi
elif [ -f "$BUILD_DIR/libpome.dylib" ]; then
    cp "$BUILD_DIR/libpome.dylib" /usr/local/lib/
    chmod 755 /usr/local/lib/libpome.dylib
fi

echo "[3/4] Installing Headers..."
mkdir -p /usr/local/include/pome
cp include/*.h /usr/local/include/pome/

# Create system module directory
mkdir -p /usr/local/lib/pome/modules
chmod 755 /usr/local/lib/pome/modules

echo "[4/4] Installing Assets..."
# Install icon and desktop file if on Linux
if [ -f "$BUILD_DIR/pome.desktop" ]; then
    mkdir -p /usr/share/applications
    cp "$BUILD_DIR/pome.desktop" /usr/share/applications/
fi
if [ -f "$BUILD_DIR/pome.png" ]; then
    mkdir -p /usr/share/icons/hicolor/128x128/apps
    cp "$BUILD_DIR/pome.png" /usr/share/icons/hicolor/128x128/apps/pome.png
fi

echo "=== Installation Complete! ==="
echo "You can now run 'pome' from anywhere."
echo "Native extensions can include <pome/pome_interpreter.h>"
