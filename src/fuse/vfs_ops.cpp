#define FUSE_USE_VERSION 30

#include "vfs_ops.h"
#include "version_manager.h"
#include "../common/paths.h"

#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string>
#include <map>

// Track if we've versioned a file in this write session
static std::map<std::string, bool> write_session_versioned;

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
    
    std::cerr << "[VFS] Initializing..." << std::endl;
    
    char *env_root = getenv("VFS_BACKEND_ROOT");
    std::string backend_root = env_root ? std::string(env_root) : "./runtime/data";
    
    size_t pos = backend_root.rfind("/data");
    std::string project_root = (pos != std::string::npos) 
        ? backend_root.substr(0, pos) 
        : backend_root + "/..";
    
    std::string versions_dir = project_root + "/versions";
    std::string meta_dir = project_root + "/meta";
    
    VersionManager::init(versions_dir, meta_dir);
    
    std::cerr << "[VFS] ✓ Versioning enabled" << std::endl;
    std::cerr << "[VFS] Backend: " << backend_root << std::endl;
    std::cerr << "[VFS] Versions: " << versions_dir << std::endl;
    
    return nullptr;
}

int vfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    std::memset(stbuf, 0, sizeof(struct stat));
    std::string real = vfs_backend_path(path);

    if (std::string(path) == "/") {
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
    std::string real = vfs_backend_path(path);

    DIR *dp = opendir(real.c_str());
    if (!dp) return -errno;

    struct dirent *de;
    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);

    while ((de = readdir(dp)) != nullptr) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        struct stat st;
        std::string child = real + "/" + de->d_name;
        if (stat(child.c_str(), &st) == -1) continue;
        filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS);
    }
    closedir(dp);
    return 0;
}

int vfs_open(const char *path, struct fuse_file_info *fi) {
    std::string real = vfs_backend_path(path);

    int flags = 0;
    if (fi->flags & O_RDONLY) flags = O_RDONLY;
    if (fi->flags & O_WRONLY) flags = O_WRONLY;
    if (fi->flags & O_RDWR)   flags = O_RDWR;
    if (fi->flags & O_APPEND) flags |= O_APPEND;

    // If opening for write, prepare for versioning
    if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
        struct stat st;
        if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
            // File exists with content - create version BEFORE any writes
            std::cerr << "[VFS] File opened for writing: " << path << std::endl;
            VersionManager::create_version(real);
            std::cerr << "[VFS] ✓ Version created on open" << std::endl;
        }
    }

    int fd = open(real.c_str(), flags);
    if (fd == -1) return -errno;
    
    fi->fh = fd;
    return 0;
}

int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    std::string real = vfs_backend_path(path);
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
    std::string real = vfs_backend_path(path);
    int fd;
    
    if (fi && fi->fh) {
        fd = fi->fh;
    } else {
        fd = open(real.c_str(), O_WRONLY);
        if (fd == -1) return -errno;
    }
    
    ssize_t res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;
    
    if (!fi || !fi->fh) close(fd);
    
    return res;
}

int vfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) fi;
    std::string real = vfs_backend_path(path);
    
    // Create version before truncating if file has content
    struct stat st;
    if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
        std::cerr << "[VFS] Truncate called, creating version..." << std::endl;
        VersionManager::create_version(real);
    }
    
    if (truncate(real.c_str(), size) == -1) return -errno;
    return 0;
}

int vfs_flush(const char *path, struct fuse_file_info *fi) {
    (void) path;
    (void) fi;
    // Flush is called when file is closed - just succeed
    return 0;
}

int vfs_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    if (fi && fi->fh) {
        close(fi->fh);
    }
    return 0;
}

int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    std::string real = vfs_backend_path(path);
    std::string parent = real.substr(0, real.find_last_of('/'));
    struct stat s;
    if (stat(parent.c_str(), &s) == -1) mkdir(parent.c_str(), 0755);
    
    int fd = open(real.c_str(), O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd == -1) return -errno;
    
    fi->fh = fd;
    
    std::cout << "[VFS] Created file: " << path << std::endl;
    return 0;
}

int vfs_unlink(const char *path) {
    std::string real = vfs_backend_path(path);
    
    // Create final version before deletion
    struct stat st;
    if (stat(real.c_str(), &st) == 0 && st.st_size > 0) {
        VersionManager::create_version(real);
        std::cout << "[VFS] Final version created before deletion: " << path << std::endl;
    }
    
    if (unlink(real.c_str()) == -1) return -errno;
    return 0;
}

int vfs_mkdir(const char *path, mode_t mode) {
    std::string real = vfs_backend_path(path);
    if (mkdir(real.c_str(), mode) == -1) return -errno;
    return 0;
}

int vfs_rmdir(const char *path) {
    std::string real = vfs_backend_path(path);
    if (rmdir(real.c_str()) == -1) return -errno;
    return 0;
}

int vfs_rename(const char *from, const char *to, unsigned int flags) {
    (void) flags;
    std::string real_from = vfs_backend_path(from);
    std::string real_to = vfs_backend_path(to);
    
    // Create version of the source file before rename
    struct stat st;
    if (stat(real_from.c_str(), &st) == 0 && st.st_size > 0) {
        VersionManager::create_version(real_from);
    }
    
    if (rename(real_from.c_str(), real_to.c_str()) == -1) return -errno;
    return 0;
}