#!/usr/bin/env bash
set -e
MOUNTPOINT=${1:-/tmp/vfs_mount}
fusermount3 -u "$MOUNTPOINT" || fusermount -u "$MOUNTPOINT"
echo "Unmounted $MOUNTPOINT"
