#pragma once

#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <sys/stat.h>
#include <string>

extern struct fuse_operations vfs_ops;
void setup_operations();

void* vfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

int vfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

int vfs_open(const char *path, struct fuse_file_info *fi);

int vfs_read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi);

int vfs_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);

int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int vfs_unlink(const char *path);

int vfs_mkdir(const char *path, mode_t mode);

int vfs_rmdir(const char *path);

int vfs_rename(const char *from, const char *to, unsigned int flags);

int vfs_truncate(const char *path, off_t size, struct fuse_file_info *fi);