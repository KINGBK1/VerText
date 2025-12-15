#!/usr/bin/env bash
set -e

# Default mount point - you can change this to anywhere you want
MOUNTPOINT=${1:-/tmp/vfs_mount}

# Get the project root (where this script is located)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build"
BIN="$BUILD_DIR/vfs_mount"

# Check if binary exists
if [ ! -x "$BIN" ]; then
  echo "Error: Build not found at $BIN"
  echo "Please run: cd build && cmake .. && make"
  exit 1
fi

# Create mount point if it doesn't exist
mkdir -p "$MOUNTPOINT"

# Create runtime/data directory if it doesn't exist
mkdir -p "$PROJECT_ROOT/runtime/data"
mkdir -p "$PROJECT_ROOT/runtime/meta"

# Export the backend root so the VFS knows where to store files
export VFS_BACKEND_ROOT="$PROJECT_ROOT/runtime/data"

# Check if already mounted
if mountpoint -q "$MOUNTPOINT" 2>/dev/null; then
    echo "Error: $MOUNTPOINT is already mounted"
    echo "Run: ./scripts/unmount.sh $MOUNTPOINT"
    exit 1
fi

echo "=========================================="
echo "Versioned VFS Mount Script"
echo "=========================================="
echo "Mount point:    $MOUNTPOINT"
echo "Backend storage: $VFS_BACKEND_ROOT"
echo "Binary:         $BIN"
echo "=========================================="
echo ""

# Decide whether to run in foreground or background
if [ "$2" == "-f" ] || [ "$2" == "--foreground" ]; then
    echo "Running in FOREGROUND mode (Ctrl+C to stop)..."
    echo "Log output will appear here."
    echo ""
    "$BIN" -f "$MOUNTPOINT"
else
    echo "Running in BACKGROUND mode..."
    echo "Log: /tmp/vfs_mount.log"
    echo ""
    
    # Run in background and redirect output
    "$BIN" "$MOUNTPOINT" > /tmp/vfs_mount.log 2>&1 &
    
    # Wait a moment to check if it started successfully
    sleep 1
    
    if mountpoint -q "$MOUNTPOINT" 2>/dev/null; then
        echo "✓ VFS mounted successfully!"
        echo ""
        echo "Try these commands:"
        echo "  ls $MOUNTPOINT"
        echo "  echo 'test' > $MOUNTPOINT/test.txt"
        echo "  cat $MOUNTPOINT/test.txt"
        echo ""
        echo "To unmount: ./scripts/unmount.sh $MOUNTPOINT"
    else
        echo "✗ Mount failed. Check /tmp/vfs_mount.log for errors"
        exit 1
    fi
fi