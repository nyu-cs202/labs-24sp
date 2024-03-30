#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#include "disk_map.h"
#include "panic.h"
#include "passert.h"
#include "dir.h"
#include "inode.h"

// Try to find a file named "name" in dir.  If so, set *ino to it and
// set *dent to the directory entry associated with the file.
//
// Returns 0 and sets *ino, *dent on success, < 0 on error.  Errors are:
//	-ENOENT if the file is not found
int
dir_lookup(struct inode *dir, const char *name, struct dirent **dent, struct inode **ino)
{
	int r;
	uint32_t i, j, nblock;
	char *blk;
	struct dirent *d;

	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->i_size % BLKSIZE) == 0);
	nblock = dir->i_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = inode_get_block(dir, i, &blk)) < 0)
			return r;
		d = (struct dirent*) blk;
		for (j = 0; j < BLKDIRENTS; j++)
			if (strcmp(d[j].d_name, name) == 0) {
				*ino = diskblock2memaddr(d[j].d_inum);
				*dent = &d[j];
				return 0;
			}
	}
	return -ENOENT;
}

// Set *dent to point to a newly-allocated dirent structure in dir.  The
// caller is responsible for filling in the dirent fields.
//
// Returns 0 and sets *dent on success, < 0 on error.
int
dir_alloc_dirent(struct inode *dir, struct dirent **dent)
{
	int r;
	uint32_t nblock, i, j;
	char *blk;
	struct dirent *d;

	assert((dir->i_size % BLKSIZE) == 0);
	nblock = dir->i_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = inode_get_block(dir, i, &blk)) < 0)
			return r;
		d = (struct dirent*) blk;
		for (j = 0; j < BLKDIRENTS; j++)
			if (d[j].d_name[0] == '\0') {
				*dent = &d[j];
				return 0;
			}
	}
	dir->i_size += BLKSIZE;
	if ((r = inode_get_block(dir, i, &blk)) < 0)
		return r;
	d = (struct dirent*) blk;
	*dent = &d[0];
	return 0;
}

// Skip over slashes.
static const char *
skip_slash(const char *p)
{
	while (*p == '/')
		p++;
	return p;
}

// Evaluate a path name, starting at the root.  On success, set *pino
// to the inode we found, set *pdir to the directory the file is in,
// and set *pdent to the directory entry in pdir associated with the file.
//
// If we cannot find the file but find the directory it should be in,
// set *pdir and copy the final path element into lastelem.
//
// Returns 0 and sets non-NULL parameters on success, < 0 on failure.
int
walk_path(const char *path, struct inode **pdir, struct inode **pino, struct dirent **pdent, char *lastelem)
{
	const char *p;
	char name[NAME_MAX];
	struct inode *dir, *ino;
	struct dirent *dent;
	int r;

	if (strlen(path) >= PATH_MAX)
		return -ENAMETOOLONG;

	// if (*path != '/')
	//	return -E_BAD_PATH;
	path = skip_slash(path);
	ino = diskblock2memaddr(super->s_root);
	dir = 0;
	dent = 0;
	name[0] = 0;

	if (pdir)
		*pdir = 0;
	if (pino)
		*pino = 0;
	if (pdent)
		*pdent = 0;
	while (*path != '\0') {
		dir = ino;
		p = path;
		while (*path != '/' && *path != '\0')
			path++;
		if (path - p >= NAME_MAX)
			return -EINVAL;
		memmove(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (!S_ISDIR(dir->i_mode))
			return -ENOENT;

		if ((r = dir_lookup(dir, name, &dent, &ino)) < 0) {
			if (r == -ENOENT && *path == '\0') {
				if (pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				if (pino)
					*pino = 0;
				if (pdent)
					*pdent = 0;
			}
			return r;
		}
	}

	if (pdir)
		*pdir = dir;
	if (pino)
		*pino = ino;
	if (pdent)
		*pdent = dent;
	return 0;
}
