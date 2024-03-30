#pragma once

#include "fs_types.h"

int	walk_path(const char *path, struct inode **pdir, struct inode **pino, struct dirent **pdent, char *lastelem);
int	dir_lookup(struct inode *dir, const char *name, struct dirent **pdent, struct inode **pino);
int	dir_alloc_dirent(struct inode *dir, struct dirent **pdent);
int	dir_alloc_inode(struct inode *dir, struct inode **pino, struct dirent **pdent);
