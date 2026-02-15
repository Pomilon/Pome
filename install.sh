#!/bin/bash

set -e

echo "=== Pome Local Installer ==="

# Directories
INSTALL_DIR="$HOME/.pome"
BIN_DIR="$HOME/.local/bin"
LIB_DIR="$INSTALL_DIR/lib"
INCLUDE_DIR="$INSTALL_DIR/include"
MODULES_DIR="$INSTALL_DIR/modules"

BUILD_DIR="build_new"

echo "[1/4] Building Pome..."
# Ensure build_new exists and is configured for Release
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

echo "[2/4] Installing to $INSTALL_DIR..."
mkdir -p "$BIN_DIR"
mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"
mkdir -p "$MODULES_DIR"

# Install binaries
cp "$BUILD_DIR/pome" "$BIN_DIR/"
cp "$BUILD_DIR/pome-lsp" "$BIN_DIR/"
cp "$BUILD_DIR/pome-fmt" "$BIN_DIR/"
chmod +x "$BIN_DIR/pome" "$BIN_DIR/pome-lsp" "$BIN_DIR/pome-fmt"

# Install library
if [ -f "$BUILD_DIR/libpome.so" ]; then
    cp "$BUILD_DIR/libpome.so" "$LIB_DIR/"
elif [ -f "$BUILD_DIR/libpome.dylib" ]; then
    cp "$BUILD_DIR/libpome.dylib" "$LIB_DIR/"
fi

echo "[3/4] Installing Headers..."
cp include/*.h "$INCLUDE_DIR/"

echo "[4/4] Finalizing..."

echo ""
echo "=== Installation Complete! ==="
echo "Binaries installed to: $BIN_DIR"
echo "Resources installed to: $INSTALL_DIR"
echo ""
echo "Make sure '$BIN_DIR' is in your PATH."
echo "Example: export PATH=\"$PATH:$BIN_DIR\""
echo ""
echo "You can now run 'pome', 'pome-lsp', and 'pome-fmt'."