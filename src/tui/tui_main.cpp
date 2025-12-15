#include "tui_manager.h"
#include "../fuse/version_manager.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Get backend root from environment or argument
    std::string backend_root;
    
    if (argc > 1) {
        backend_root = argv[1];
        setenv("VFS_BACKEND_ROOT", backend_root.c_str(), 1);
    } else {
        char* env_root = getenv("VFS_BACKEND_ROOT");
        backend_root = env_root ? std::string(env_root) : "./runtime/data";
    }
    
    // Initialize version manager
    size_t pos = backend_root.rfind("/data");
    std::string project_root = (pos != std::string::npos) 
        ? backend_root.substr(0, pos) 
        : backend_root + "/..";
    
    std::string versions_dir = project_root + "/versions";
    std::string meta_dir = project_root + "/meta";
    
    VersionManager::init(versions_dir, meta_dir);
    
    try {
        TUIManager tui;
        tui.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}