#!/usr/bin/env bash
set -e

echo "=========================================="
echo "Versioned VFS Setup Script"
echo "=========================================="

# Get project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Create necessary directories
echo "Creating directories..."
mkdir -p build
mkdir -p runtime/data
mkdir -p runtime/meta
mkdir -p runtime/versions

# Configure with CMake
echo ""
echo "Running CMake..."
cd build
cmake ..

# Build
echo ""
echo "Building project..."
make

echo ""
echo "=========================================="
echo "✓ Build complete!"
echo "=========================================="
echo ""
echo "Executable: $PROJECT_ROOT/build/vfs_mount"
echo ""
echo "Next steps:"
echo "  1. Mount the filesystem:"
echo "     ./scripts/mount.sh /tmp/vfs_mount"
echo ""
echo "  2. Or mount to a custom location:"
echo "     ./scripts/mount.sh /mnt/my_vfs"
echo ""
echo "  3. Test it:"
echo "     echo 'hello' > /tmp/vfs_mount/test.txt"
echo "     cat /tmp/vfs_mount/test.txt"
echo ""
echo "  4. Unmount when done:"
echo "     ./scripts/unmount.sh /tmp/vfs_mount"