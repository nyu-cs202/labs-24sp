#pragma once

#include <sys/stat.h>

#include "fs_types.h"

int	inode_block_walk(struct inode *ino, uint32_t filebno, uint32_t **ppdiskbno, bool alloc);
int	inode_get_block(struct inode *ino, uint32_t file_blockno, char **pblk);
int	inode_create(const char *path, struct inode **ino);
int	inode_open(const char *path, struct inode **ino);
ssize_t	inode_read(struct inode *ino, void *buf, size_t count, uint32_t offset);
int	inode_write(struct inode *ino, const void *buf, size_t count, uint32_t offset);
int	inode_set_size(struct inode *ino, uint32_t newsize);
void	inode_flush(struct inode *ino);
int	inode_unlink(const char *path);
int	inode_link(const char *srcpath, const char *dstpath);
int	inode_stat(struct inode *ino, struct stat *stbuf);
