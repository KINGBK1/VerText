#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <iostream>
#include "vfs_ops.h"

using namespace std;

extern struct fuse_operations vfs_ops;   // Declare ops table
void setup_operations();                 // Declare setup function

int main(int argc, char *argv[]) {
    cout << "Starting Versioned VFS" << endl;

    // Register all FUSE operations
    setup_operations();

    // Start FUSE with our handlers
    return fuse_main(argc, argv, &vfs_ops, nullptr);
}
