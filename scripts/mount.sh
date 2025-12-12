#!/usr/bin/env bash
set -e
MOUNTPOINT=${1:-/tmp/vfs_mount}
BUILD_DIR="$(cd "$(dirname "$0")/.." && pwd)/build"
BIN="$BUILD_DIR/vfs_mount"

if [ ! -x "$BIN" ]; then
  echo "Build not found. Run cmake && make first."
  exit 1
fi

mkdir -p "$MOUNTPOINT"
echo "Mounting VFS at $MOUNTPOINT (run in foreground with -f for debug)..."
# run foreground for easier debugging
"$BIN" -f "$MOUNTPOINT"
