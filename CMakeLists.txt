cmake_minimum_required(VERSION 3.10)
project(versioned_vfs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable useful warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Find FUSE3
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

# Find ncurses (for TUI)
find_package(Curses REQUIRED)

include_directories(
    ${FUSE3_INCLUDE_DIRS}
    ${CURSES_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

link_directories(
    ${FUSE3_LIBRARY_DIRS}
)

# Source files for VFS mount
set(VFS_SOURCES
    src/common/paths.cpp
    src/fuse/vfs_main.cpp
    src/fuse/vfs_ops.cpp
    src/fuse/version_manager.cpp
)

# Source files for TUI
set(TUI_SOURCES
    src/tui/tui_main.cpp
    src/tui/tui_manager.cpp
    src/common/paths.cpp
    src/fuse/version_manager.cpp
)

# Create the VFS mount executable
add_executable(vfs_mount ${VFS_SOURCES})
target_link_libraries(vfs_mount
    ${FUSE3_LIBRARIES}
)

# Create the TUI executable
add_executable(vfs_tui ${TUI_SOURCES})
target_link_libraries(vfs_tui
    ${CURSES_LIBRARIES}
)

# Install rule (optional)
install(TARGETS vfs_mount vfs_tui DESTINATION bin)