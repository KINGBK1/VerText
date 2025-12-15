#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string>
#include <map>

#include "vfs_ops.h"
#include "version_manager.h"
#include "../common/paths.h"

using namespace std;

// Track open files and whether they've been modified
struct OpenFileInfo {
    bool has_been_written;
    bool version_created;
    off_t original_size;
};

static map<string, OpenFileInfo> open_files;

struct fuse_operations vfs_ops = {};

void setup_operations() {
    vfs_ops.init    = vfs_init;
    vfs_ops.getattr = vfs_getattr;
    vfs_ops.readdir = vfs_readdir;
    vfs_ops.open    = vfs_open;
    vfs_ops.read    = vfs_read;
    vfs_ops.write   = vfs_write;
    vfs_ops.create  = vfs_create;
    vfs_ops.unlink  = vfs_unlink;
    vfs_ops.mkdir   = vfs_mkdir;
    vfs_ops.rmdir   = vfs_rmdir;
    vfs_ops.rename  = vfs_rename;
    vfs_ops.truncate = vfs_truncate;
    vfs_ops.flush   = vfs_flush;
    vfs_ops.release = vfs_release;
}

void* vfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    (void) cfg;
    
    cerr << "[VFS] ═══════════════════════════════════════" << endl;
    cerr << "[VFS] Versioned Filesystem Initializing..." << endl;
    cerr << "[VFS] ═══════════════════════════════════════" << endl;
    
    char *env_root = getenv("VFS_BACKEND_ROOT");
    string backend_root = env_root ? string(env_root) : "./runtime/data";
    
    size_t pos = backend_root.rfind("/data");
    string project_root = (pos != string::npos) 
        ? backend_root.substr(0, pos) 
        : backend_root + "/..";
    
    string versions_dir = project_root + "/versions";
    string meta_dir = project_root + "/meta";
    
    VersionManager::init(versions_dir, meta_dir);
    
    cerr << "[VFS] ✓ Versioning System: ACTIVE" << endl;
    cerr << "[VFS] ✓ Backend Storage:   " << backend_root << endl;
    cerr << "[VFS] ✓ Version Archive:   " << versions_dir << endl;
    cerr << "[VFS] ✓ Metadata Storage:  " << meta_dir << endl;
    cerr << "[VFS] ═══════════════════════════════════════" << endl;
    cerr << "[VFS] Ready! All file changes will be versioned." << endl;
    
    return nullptr;
}

int vfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    string real = vfs_backend_path(path);

    if (string(path) == "/") {
        struct stat s;
        if (stat(real.c_str(), &s) == -1) {
            mkdir(real.c_str(), 0755);
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }

    if (stat(real.c_str(), stbuf) == -1) return -errno;
    return 0;
}

int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    string real = vfs_backend_path(path);

    DIR *dp = opendir(real.c_str());
    if (!dp) return -errno;

    struct dirent *de;
    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);

    while ((de = readdir(dp)) != nullptr) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        struct stat st;
        string child = real + "/" + de->d_name;
        if (stat(child.c_str(), &st) == -1) continue;
        filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS);
    }
    closedir(dp);
    return 0;
}

int vfs_open(const char *path, struct fuse_file_info *fi) {
    string real = vfs_backend_path(path);

    int flags = 0;
    if (fi->flags & O_RDONLY) flags = O_RDONLY;
    if (fi->flags & O_WRONLY) flags = O_WRONLY;
    if (fi->flags & O_RDWR)   flags = O_RDWR;
    if (fi->flags & O_APPEND) flags |= O_APPEND;
    if (fi->flags & O_TRUNC)  flags |= O_TRUNC;

    // Create version BEFORE opening if truncate flag is set
    if ((fi->flags & O_TRUNC) && (fi->flags & (O_WRONLY | O_RDWR))) {
        struct stat st;
        if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
            cerr << "[VFS] Truncate on open detected: " << path << endl;
            VersionManager::create_version(real);
            cerr << "[VFS] ✓ Version created before truncate!" << endl;
        }
    }

    int fd = open(real.c_str(), flags);
    if (fd == -1) return -errno;
    
    fi->fh = fd;
    
    // Track this file if opened for writing
    if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
        OpenFileInfo info;
        info.has_been_written = false;
        info.version_created = (fi->flags & O_TRUNC) ? true : false; // Already versioned if truncated
        
        struct stat st;
        if (stat(real.c_str(), &st) == 0) {
            info.original_size = st.st_size;
        } else {
            info.original_size = 0;
        }
        
        open_files[real] = info;
        cerr << "[VFS] Opened for writing: " << path << " (flags: " << fi->flags << ", size: " << info.original_size << ")" << endl;
    }

    return 0;
}

int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    string real = vfs_backend_path(path);
    int fd;
    
    if (fi && fi->fh) {
        fd = fi->fh;
    } else {
        fd = open(real.c_str(), O_RDONLY);
        if (fd == -1) return -errno;
    }
    
    ssize_t res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    
    if (!fi || !fi->fh) close(fd);
    
    return res;
}

int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    string real = vfs_backend_path(path);
    int fd;
    
    if (fi && fi->fh) {
        fd = fi->fh;
    } else {
        fd = open(real.c_str(), O_WRONLY);
        if (fd == -1) return -errno;
    }
    
    // Before first write, create version if file has content AND we haven't already versioned it
    auto it = open_files.find(real);
    if (it != open_files.end() && !it->second.version_created && !it->second.has_been_written) {
        struct stat st;
        if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
            cerr << "[VFS] Creating version before first write: " << path << endl;
            VersionManager::create_version(real);
            it->second.version_created = true;
            cerr << "[VFS] ✓ Version created successfully!" << endl;
        }
        it->second.has_been_written = true;
    }
    
    ssize_t res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;
    
    if (!fi || !fi->fh) close(fd);
    
    return res;
}

int vfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) fi;
    string real = vfs_backend_path(path);
    
    // Create version before truncating if file has content
    struct stat st;
    if (stat(real.c_str(), &st) == 0 && st.st_size > 0 && size < st.st_size) {
        cerr << "[VFS] Truncate detected, creating version: " << path << endl;
        VersionManager::create_version(real);
        cerr << "[VFS] ✓ Version saved before truncation" << endl;
    }
    
    if (truncate(real.c_str(), size) == -1) return -errno;
    return 0;
}

int vfs_flush(const char *path, struct fuse_file_info *fi) {
    (void) path;
    (void) fi;
    return 0;
}

int vfs_release(const char *path, struct fuse_file_info *fi) {
    string real = vfs_backend_path(path);
    
    // Check if file was modified but version wasn't created
    auto it = open_files.find(real);
    if (it != open_files.end()) {
        if (it->second.has_been_written && !it->second.version_created) {
            struct stat st;
            if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
                cerr << "[VFS] 💾 Creating version on close: " << path << endl;
                VersionManager::create_version(real);
                cerr << "[VFS] ✓ Final version saved!" << endl;
            }
        }
        open_files.erase(it);
    }
    
    if (fi && fi->fh) {
        close(fi->fh);
    }
    return 0;
}

int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    string real = vfs_backend_path(path);
    string parent = real.substr(0, real.find_last_of('/'));
    struct stat s;
    if (stat(parent.c_str(), &s) == -1) mkdir(parent.c_str(), 0755);
    
    int fd = open(real.c_str(), O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd == -1) return -errno;
    
    fi->fh = fd;
    
    // Track this new file
    OpenFileInfo info;
    info.has_been_written = false;
    info.version_created = false;
    info.original_size = 0;
    open_files[real] = info;
    
    cerr << "[VFS] New file created: " << path << endl;
    return 0;
}

int vfs_unlink(const char *path) {
    string real = vfs_backend_path(path);
    
    // Create final version before deletion
    struct stat st;
    if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
        cerr << "[VFS] 🗑️ Creating final version before deletion: " << path << endl;
        VersionManager::create_version(real);
        cerr << "[VFS] ✓ Final version preserved!" << endl;
    }
    
    if (unlink(real.c_str()) == -1) return -errno;
    return 0;
}

int vfs_mkdir(const char *path, mode_t mode) {
    string real = vfs_backend_path(path);
    if (mkdir(real.c_str(), mode) == -1) return -errno;
    return 0;
}

int vfs_rmdir(const char *path) {
    string real = vfs_backend_path(path);
    if (rmdir(real.c_str()) == -1) return -errno;
    return 0;
}

int vfs_rename(const char *from, const char *to, unsigned int flags) {
    (void) flags;
    string real_from = vfs_backend_path(from);
    string real_to = vfs_backend_path(to);
    
    // Create version of the source file before rename
    struct stat st;
    if (stat(real_from.c_str(), &st) == 0 && st.st_size > 0) {
        cerr << "[VFS] Creating version before rename: " << from << " -> " << to << endl;
        VersionManager::create_version(real_from);
    }
    
    if (rename(real_from.c_str(), real_to.c_str()) == -1) return -errno;
    return 0;
}