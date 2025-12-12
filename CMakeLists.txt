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

# Recursively collect all .cpp files inside src/
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Create the executable
add_executable(vfs_mount ${SOURCES})

# Link libraries
target_link_libraries(vfs_mount
    ${FUSE3_LIBRARIES}
    ${CURSES_LIBRARIES}
)

# Install rule (optional)
install(TARGETS vfs_mount DESTINATION bin)
