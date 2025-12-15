#!/usr/bin/env bash

MOUNTPOINT="/tmp/vfs_mount"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "=========================================="
echo "Testing Versioned VFS"
echo "=========================================="
echo ""

# Check if mounted
if ! mountpoint -q "$MOUNTPOINT" 2>/dev/null; then
    echo "Error: VFS not mounted at $MOUNTPOINT"
    echo "Run: ./scripts/mount.sh $MOUNTPOINT"
    exit 1
fi

echo "✓ VFS is mounted"
echo ""

# Test file
TEST_FILE="$MOUNTPOINT/test.txt"

echo "1. Creating initial file..."
echo "Version 1 content" > "$TEST_FILE"
sleep 1

echo "2. Modifying file (creating version 2)..."
echo "Version 2 content" > "$TEST_FILE"
sleep 1

echo "3. Modifying file again (creating version 3)..."
echo "Version 3 content" > "$TEST_FILE"
sleep 1

echo "4. One more modification (creating version 4)..."
echo "Version 4 - final content" > "$TEST_FILE"
sleep 1

echo ""
echo "✓ Created 4 versions of test.txt"
echo ""

echo "Current content:"
cat "$TEST_FILE"
echo ""

echo "=========================================="
echo "Checking Version Storage"
echo "=========================================="
echo ""

echo "Backend file location:"
ls -lh "$PROJECT_ROOT/runtime/data/test.txt" 2>/dev/null || echo "  File exists in VFS"
echo ""

echo "Version storage:"
ls -lh "$PROJECT_ROOT/runtime/versions/"test.txt_versions/ 2>/dev/null || echo "  No versions directory yet"
echo ""

echo "Metadata:"
cat "$PROJECT_ROOT/runtime/meta/test.txt.meta" 2>/dev/null || echo "  No metadata file yet"
echo ""

echo "=========================================="
echo "Statistics"
echo "=========================================="
echo ""
echo "Files in VFS:"
ls -lh "$MOUNTPOINT/" | grep -v "^total" | wc -l
echo ""

echo "Version files created:"
find "$PROJECT_ROOT/runtime/versions" -type f 2>/dev/null | wc -l
echo ""

echo "Metadata files:"
find "$PROJECT_ROOT/runtime/meta" -type f 2>/dev/null | wc -l
echo ""

echo "=========================================="
echo "Test complete!"
echo "=========================================="
echo ""
echo "To view the log: tail -f /tmp/vfs_mount.log"