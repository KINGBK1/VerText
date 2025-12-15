#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN="$PROJECT_ROOT/build/vfs_tui"

# Check if binary exists
if [ ! -x "$BIN" ]; then
    echo "Error: TUI not built. Building now..."
    cd "$PROJECT_ROOT/build"
    cmake ..
    make
fi

# Set backend root
export VFS_BACKEND_ROOT="$PROJECT_ROOT/runtime/data"

# Clear screen and run TUI
clear
echo "Starting Versioned VFS TUI..."
echo "Backend: $VFS_BACKEND_ROOT"
echo ""
sleep 1

"$BIN" "$VFS_BACKEND_ROOT"