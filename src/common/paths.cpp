#include "paths.h"
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

static std::string backend_root_cached;

// Get the absolute path to runtime/data, creating it if needed
static std::string get_backend_root() {
    if (!backend_root_cached.empty()) return backend_root_cached;

    // Try environment variable first (you can set this before mounting)
    char *env_root = getenv("VFS_BACKEND_ROOT");
    if (env_root) {
        backend_root_cached = std::string(env_root);
    } else {
        // Fall back to current working directory + runtime/data
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            backend_root_cached = std::string(cwd) + "/runtime/data";
        } else {
            backend_root_cached = "./runtime/data";
        }
    }

    // Ensure the backend directory exists
    struct stat st;
    if (stat(backend_root_cached.c_str(), &st) == -1) {
        // Create the directory recursively
        std::string parent = backend_root_cached.substr(0, backend_root_cached.find_last_of('/'));
        mkdir(parent.c_str(), 0755);  // Create runtime/
        mkdir(backend_root_cached.c_str(), 0755);  // Create runtime/data/
    }

    return backend_root_cached;
}

std::string vfs_backend_path(const char *virtual_path) {
    std::string backend = get_backend_root();
    
    // Remove trailing slash from backend if present
    if (!backend.empty() && backend.back() == '/') {
        backend.pop_back();
    }

    // Handle the virtual path
    std::string vp(virtual_path);
    
    // Root directory case
    if (vp == "/") {
        return backend;
    }

    // Remove leading slash if present (we'll add it back)
    if (!vp.empty() && vp.front() == '/') {
        vp.erase(0, 1);
    }

    // Return the combined path
    if (vp.empty()) {
        return backend;
    }
    return backend + "/" + vp;
}