#include "paths.h"
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

static std::string project_root_cached;

static std::string get_project_root() {
    if (!project_root_cached.empty()) return project_root_cached;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        project_root_cached = std::string(cwd);
        return project_root_cached;
    }

    char *p = getenv("PWD");
    if (p) {
        project_root_cached = std::string(p);
        return project_root_cached;
    }
    return std::string(".");
}

std::string vfs_backend_path(const char *virtual_path) {
    std::string root = get_project_root();
    if (!root.empty() && root.back() == '/') root.pop_back();

    std::string vp(virtual_path);
    if (vp.size() > 1 && vp.front() == '/') vp.erase(0, 1);

    std::string backend = root + "/runtime/data";
    if (vp.empty()) return backend + "/";
    return backend + "/" + vp;
}
