# VerText (Versioned VFS)

VerText is a proof-of-concept Versioned Virtual File System implemented in C++ using FUSE (Filesystem in Userspace). It allows you to mount a virtual directory where every file modification is tracked, essentially providing git-like versioning capabilities directly at the filesystem level.

The project also includes a TUI (Text User Interface) tool to inspect the version history and manage the underlying data.

## Features
- **FUSE-based Mounting**: Mount the filesystem and inspect it with standard tools (`ls`, `cat`, `nano`, etc.)
- **Automatic Versioning**: Writes to files automatically create new versions.
- **TUI Inspector**: An ncurses-based terminal UI to view version history and backend storage layout.
- **Backend Storage**: Uses a structured directory layout in `runtime/data` to store file blobs and metadata.

---

## Prerequisites

Before building, ensure you have the following installed:

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake libfuse3-dev libncurses-dev pkg-config
```

### macOS
You need to install [FUSE for macOS](https://osxfuse.github.io/) (now macFUSE) or `fuse-t`.
```bash
brew install cmake pkg-config
brew install --cask macfuse
# You may also need to install ncurses if not present (usually pre-installed)
```

### Windows
**Note:** FUSE is not natively supported on Windows. To run this project on Windows, you **MUST** use [WSL2 (Windows Subsystem for Linux)](https://learn.microsoft.com/en-us/windows/wsl/install).
1. Install WSL2 (`wsl --install` in PowerShell).
2. Open your Ubuntu (or other distro) terminal.
3. Follow the **Linux** instructions above inside the WSL terminal.

---

## Build Instructions

1. **Clone or Navigate to the project directory**:
   ```bash
   cd VerText
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure and Build**:
   ```bash
   cmake ..
   make
   ```
   This will generate two executables in the `build/` directory:
   - `vfs_mount`: The FUSE daemon.
   - `vfs_tui`: The TUI version inspector.

---

## How to Run

The project provides convenient helper scripts in the `scripts/` directory to manage mounting and running applications.

### 1. Mount the Filesystem
To start the VFS, use the `mount.sh` script. By default, it mounts to `/tmp/vfs_mount`.

```bash
# Run in foreground (helpful for debugging/logging)
./scripts/mount.sh <path to the directory where you wanna mount> -f

# OR Run in background
./scripts/mount.sh
```
*Note: The script also sets up the necessary `runtime/` directories for storage.*

Once mounted, you can interact with it like a normal folder:
```bash
cd /tmp/vfs_mount
echo "Hello World" > hello.txt
cat hello.txt
```

### 2. Run the TUI Inspector
To view the backend storage and version history, use the TUI script. **Note: You can run this even while the VFS is mounted.**

```bash
./scripts/run_tui.sh
```
Use the arrow keys to navigate and `q` to quit.

### 3. Unmount
When you are done, unmount the filesystem properly to ensure data is flushed and the daemon stops.

```bash
./scripts/unmount.sh
```

---

## Project Structure

- `src/fuse/`: Core FUSE implementation (operations, main loop).
- `src/tui/`: Ncurses-based TUI implementation.
- `src/common/`: Shared utilities (path handling).
- `scripts/`: Helper scripts for mounting/unmounting.
- `runtime/`: Created at runtime.
    - `data/`: Backend blob storage.
    - `meta/`: Metadata storage.
