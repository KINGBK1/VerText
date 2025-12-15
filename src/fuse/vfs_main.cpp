#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <iostream>
#include "vfs_ops.h"

using namespace std;

extern struct fuse_operations vfs_ops;
void setup_operations();

int main(int argc, char *argv[]) {
    cout << "╔═══════════════════════════════════════╗" << endl;
    cout << "║   VERSIONED VFS - Starting System...  ║" << endl;
    cout << "╚═══════════════════════════════════════╝" << endl;
    cout << endl;

    setup_operations();

    return fuse_main(argc, argv, &vfs_ops, nullptr);
}