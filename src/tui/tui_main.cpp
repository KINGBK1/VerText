#include "tui_manager.h"
#include "../fuse/version_manager.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[]) {
    // Get backend root from environment or argument
    string backend_root;
    
    if (argc > 1) {
        backend_root = argv[1];
        setenv("VFS_BACKEND_ROOT", backend_root.c_str(), 1);
    } else {
        char* env_root = getenv("VFS_BACKEND_ROOT");
        backend_root = env_root ? string(env_root) : "./runtime/data";
    }
    
    // Initialize version manager
    size_t pos = backend_root.rfind("/data");
    string project_root = (pos != string::npos) 
        ? backend_root.substr(0, pos) 
        : backend_root + "/..";
    
    string versions_dir = project_root + "/versions";
    string meta_dir = project_root + "/meta";
    
    VersionManager::init(versions_dir, meta_dir);
    
    try {
        TUIManager tui;
        tui.run();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}