#include <fuse.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "fs_types.h"
#include "inode.h"
#include "dir.h"
#include "disk_map.h"
#include "bitmap.h"
#include "panic.h"
#include "passert.h"

int	fs_getattr(const char *path, struct stat *stbuf);
int	fs_readlink(const char *path, char *target, size_t len);
int	fs_mknod(const char *path, mode_t mode, dev_t rdev);
int	fs_mkdir(const char *path, mode_t mode);
int	fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int	fs_unlink(const char *path);
int	fs_rmdir(const char *path);
int	fs_symlink(const char *dstpath, const char *srcpath);
int	fs_rename(const char *srcpath, const char *dstpath);
int	fs_link(const char *srcpath, const char *dstpath);
int	fs_chmod(const char *path, mode_t mode);
int	fs_chown(const char *path, uid_t uid, gid_t gid);
int	fs_truncate(const char *path, off_t size);
int	fs_open(const char *path, struct fuse_file_info *fi);
int	fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int	fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int	fs_statfs(const char *path, struct statvfs *stbuf);
int	fs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int	fs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi);
int	fs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
int	fs_utimens(const char *path, const struct timespec tv[2]);
int	fs_parse_opt(void *data, const char *arg, int key, struct fuse_args *outargs);

struct fuse_operations fs_oper = {
	.getattr	= fs_getattr,
	.readlink	= fs_readlink,
	.mknod		= fs_mknod,
	.mkdir		= fs_mkdir,
	.opendir	= fs_open, // No difference between open and opendir.
	.readdir	= fs_readdir,
	.unlink		= fs_unlink,
	.rmdir		= fs_rmdir,
	.symlink	= fs_symlink,
	.rename		= fs_rename,
	.link		= fs_link,
	.chmod		= fs_chmod,
	.chown		= fs_chown,
	.truncate	= fs_truncate,
	.open		= fs_open,
	.read		= fs_read,
	.write		= fs_write,
	.statfs		= fs_statfs,
	.fsync		= fs_fsync,
	.ftruncate	= fs_ftruncate,
	.fgetattr	= fs_fgetattr,
	.utimens	= fs_utimens,
};

enum {
	KEY_VERSION,
	KEY_HELP,
	KEY_TEST_OPS,
};

static struct fuse_opt fs_opts[] = {
	FUSE_OPT_KEY("-V",         KEY_VERSION),
	FUSE_OPT_KEY("--version",  KEY_VERSION),
	FUSE_OPT_KEY("-h",         KEY_HELP),
	FUSE_OPT_KEY("-ho",        KEY_HELP),
	FUSE_OPT_KEY("--help",     KEY_HELP),
	FUSE_OPT_KEY("--test-ops", KEY_TEST_OPS),
	FUSE_OPT_END,
};

// --------------------------------------------------------------
// Primitive file system operations test
// --------------------------------------------------------------

static char *msg = "This is a rather uninteresting message.\n\n";

void
fs_test(void)
{
	struct inode *ino, *ino2;
	int r;
	char *blk;
	uint32_t bits[4096];

	// back up bitmap
	memmove(bits, bitmap, 4096);
	// allocate block
	if ((r = alloc_block()) < 0)
		panic("alloc_block: %s", strerror(-r));
	// check that block was free
	assert(bits[r/32] & (1 << (r%32)));
	// and is not free any more
	assert(!(bitmap[r/32] & (1 << (r%32))));
	free_block(r);
	printf("alloc_block is good\n");

	if ((r = inode_open("/not-found", &ino)) < 0 && r != -ENOENT)
		panic("inode_open /not-found: %s", strerror(-r));
	else if (r == 0)
		panic("inode_open /not-found succeeded!");
	if ((r = inode_open("/msg", &ino)) < 0)
		panic("inode_open /msg: %s", strerror(-r));
	printf("inode_open is good\n");

	if ((r = inode_get_block(ino, 0, &blk)) < 0)
		panic("inode_get_block: %s", strerror(-r));
	if (strcmp(blk, msg) != 0)
		panic("inode_get_block returned wrong data");
	printf("inode_get_block is good\n");

	if ((r = inode_set_size(ino, 0)) < 0)
		panic("inode_set_size: %s", strerror(-r));
	assert(ino->i_direct[0] == 0);
	printf("inode_truncate is good\n");

	if ((r = inode_set_size(ino, strlen(msg))) < 0)
		panic("inode_set_size 2: %s", strerror(-r));
	if ((r = inode_get_block(ino, 0, &blk)) < 0)
		panic("inode_get_block 2: %s", strerror(-r));
	strcpy(blk, msg);
	printf("file rewrite is good\n");

	if ((r = inode_link("/msg", "/linkmsg")) < 0)
		panic("inode_link /msg /linkmsg: %s", strerror(-r));
	if ((r = inode_open("/msg", &ino)) < 0)
		panic("inode_open /msg: %s", strerror(-r));
	if ((r = inode_open("/linkmsg", &ino2)) < 0)
		panic("inode_open /linkmsg: %s", strerror(-r));
	if (ino != ino2)
		panic("linked files do not point to same inode");
	if (ino->i_nlink != 2)
		panic("link count incorrect: %u, expected 2", ino->i_nlink);
	printf("inode_link is good\n");

	if ((r = inode_unlink("/linkmsg")) < 0)
		panic("inode_unlink /linkmsg: %s", strerror(-r));
	if ((r = inode_open("/linkmsg", &ino2)) < 0 && r != -ENOENT)
		panic("inode_open /linkmsg after unlink: %s", strerror(-r));
	else if (r == 0)
		panic("inode_open /linkmsg after unlink succeeded!");
	if ((r = inode_open("/msg", &ino)) < 0)
		panic("inode_open /msg after /linkmsg unlinked: %s", strerror(-r));
	if (ino->i_nlink != 1)
		panic("link count incorrect: %u, expected 1", ino->i_nlink);
	printf("inode_unlink is good\n");
}

// --------------------------------------------------------------
// FUSE callbacks
// --------------------------------------------------------------

int
fs_getattr(const char *path, struct stat *stbuf)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	memset(stbuf, 0, sizeof(*stbuf));
	inode_stat(ino, stbuf);

	return 0;
}

int
fs_readlink(const char *path, char *target, size_t len)
{
	struct inode *ino;
	size_t copylen;
	char *blk;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	if ((r = inode_get_block(ino, 0, &blk)) < 0)
		return r;
	copylen = MIN(ino->i_size, len - 1);
	memcpy(target, blk, copylen);
	target[copylen] = '\0';

	return 0;
}

int
fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	struct inode *ino;
	time_t curtime;
	struct fuse_context *ctxt;
	int r;

	if ((r = inode_create(path, &ino)) < 0)
		return r;
	ino->i_size = 0;
	ino->i_mode = mode;
	ino->i_nlink = 1;
	ino->i_rdev = rdev;

	curtime = time(NULL);
	ino->i_atime = curtime;
	ino->i_ctime = curtime;
	ino->i_mtime = curtime;

	ctxt = fuse_get_context();
	ino->i_owner = ctxt->uid;
	ino->i_group = ctxt->gid;
	flush_block(ino);

	return 0;
}

int
fs_mkdir(const char *path, mode_t mode)
{
	struct inode *dir;
	time_t curtime;
	struct fuse_context *ctxt;
	int r;

	if ((r = inode_create(path, &dir)) < 0)
		return r;
	dir->i_size = 0;
	dir->i_mode = S_IFDIR | (mode & 0777);
	dir->i_nlink = 1;

	curtime = time(NULL);
	dir->i_atime = curtime;
	dir->i_ctime = curtime;
	dir->i_mtime = curtime;

	ctxt = fuse_get_context();
	dir->i_owner = ctxt->uid;
	dir->i_group = ctxt->gid;
	flush_block(dir);

	return 0;
}

int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	struct inode *dir = (struct inode *)fi->fh;
	struct dirent dent;
	int r;

	while ((r = inode_read(dir, &dent, sizeof(dent), offset)) > 0) {
		offset += r;
		if (dent.d_name[0] == '\0')
			continue;
		if (filler(buf, dent.d_name, NULL, offset) != 0)
			return 0;
	}
	dir->i_atime = time(NULL);
	flush_block(dir);

	return 0;
}

int
fs_unlink(const char *path)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	if (S_ISDIR(ino->i_mode))
		return -EISDIR;
	ino->i_ctime = time(NULL);
	return inode_unlink(path);
}

int
fs_rmdir(const char *path)
{
	struct inode *dir;
	struct dirent *dent;
	uint32_t nblock, i, j;
	char *blk;
	int r;

	if ((r = inode_open(path, &dir)) < 0)
		return r;
	if (dir == diskblock2memaddr(super->s_root))
		return -EPERM;
	if (!S_ISDIR(dir->i_mode))
		return -ENOTDIR;

	nblock = dir->i_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = inode_get_block(dir, i, &blk)) < 0)
			return r;
		dent = (struct dirent *)blk;
		for (j = 0; j < BLKDIRENTS; ++j)
			if (dent[j].d_name[0] != '\0')
				return -ENOTEMPTY;
	}
	return inode_unlink(path);
}

int
fs_symlink(const char *dstpath, const char *srcpath)
{
	struct inode *ino;
	struct fuse_context *ctxt;
	time_t curtime;
	size_t dstlen;
	char *blk;
	int r;

	if ((dstlen = strlen(dstpath)) >= PATH_MAX)
		return -ENAMETOOLONG;
	if ((r = inode_create(srcpath, &ino)) < 0) {
		return r;
	}
	ino->i_size = dstlen;
	ino->i_mode = S_IFLNK | 0777;
	ino->i_nlink = 1;

	curtime = time(NULL);
	ino->i_atime = curtime;
	ino->i_ctime = curtime;
	ino->i_mtime = curtime;

	ctxt = fuse_get_context();
	ino->i_owner = ctxt->uid;
	ino->i_group = ctxt->gid;

	if ((r = inode_get_block(ino, 0, &blk)) < 0) {
		inode_unlink(srcpath);
		return r;
	}
	memcpy(blk, dstpath, dstlen);
	inode_flush(ino);

	return 0;
}

int
fs_rename(const char *srcpath, const char *dstpath)
{
	int r;

link_retry:
	if ((r = inode_link(srcpath, dstpath)) < 0)
		switch(-r) {
		case EEXIST:
			if (strcmp(srcpath, dstpath) == 0)
				return 0;
			if ((r = inode_unlink(dstpath)) < 0)
				return r;
			goto link_retry;
		default:
			return r;
		}
	return inode_unlink(srcpath);
}

int
fs_link(const char *srcpath, const char *dstpath)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(srcpath, &ino)) < 0)
		return r;
	if (S_ISDIR(ino->i_mode))
		return -EPERM;
	ino->i_ctime = time(NULL);
	return inode_link(srcpath, dstpath);
}

int
fs_chmod(const char *path, mode_t mode)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	if (ino == diskblock2memaddr(super->s_root))
		return -EPERM;
	ino->i_mode = mode;
	ino->i_ctime = time(NULL);
	flush_block(ino);

	return 0;
}

int
fs_chown(const char *path, uid_t uid, gid_t gid)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	if (ino == diskblock2memaddr(super->s_root))
		return -EPERM;
	if (uid != -1)
		ino->i_owner = uid;
	if (gid != -1)
		ino->i_group = gid;
	ino->i_ctime = time(NULL);
	flush_block(ino);

	return 0;
}

int
fs_truncate(const char *path, off_t size)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	ino->i_mtime = time(NULL);
	return inode_set_size(ino, size);
}

int
fs_open(const char *path, struct fuse_file_info *fi)
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	fi->fh = (uint64_t)ino;
	return 0;
}

int
fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct inode *ino = (struct inode *)fi->fh;
	ino->i_atime = time(NULL);
	return inode_read(ino, buf, size, offset);
}

int
fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct inode *ino = (struct inode *)fi->fh;
	ino->i_mtime = time(NULL);
	return inode_write(ino, buf, size, offset);
}

int
fs_statfs(const char *path, struct statvfs *stbuf)
{
	int i;

	memset(stbuf, 0, sizeof(*stbuf));
	stbuf->f_bsize = BLKSIZE;
	stbuf->f_frsize = BLKSIZE;
	stbuf->f_blocks = super->s_nblocks;
	stbuf->f_fsid = super->s_magic;
	stbuf->f_namemax = PATH_MAX;
	for (i = 0; i < super->s_nblocks; ++i)
		if (block_is_free(i))
			stbuf->f_bfree++;
	stbuf->f_bavail = stbuf->f_bfree;

	return 0;
}

int
fs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	struct inode *ino = (struct inode *)fi->fh;
	inode_flush(ino);
	return 0;
}

int
fs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	struct inode *ino = (struct inode *)fi->fh;
	ino->i_mtime = time(NULL);
	return inode_set_size(ino, size);
}

int
fs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	struct inode *ino = (struct inode *)fi->fh;
	memset(stbuf, 0, sizeof(*stbuf));
	inode_stat(ino, stbuf);

	return 0;
}

int
fs_utimens(const char *path, const struct timespec tv[2])
{
	struct inode *ino;
	int r;

	if ((r = inode_open(path, &ino)) < 0)
		return r;
	ino->i_atime = tv[0].tv_sec;
	ino->i_mtime = tv[1].tv_sec;
	ino->i_ctime = time(NULL);
	flush_block(ino);

	return 0;
}

int
fs_parse_opt(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	static const char *usage_str =
"usage: fsdriver imagefile mountpoint [options]\n"
"Mount a CS202 file system image at a given mount point.\n\n"
"Special options:\n"
"    -h, -ho, --help        show this help message and exit\n"
"    --test-ops             test basic file system operations on a specific\n"
"                           disk image, but don't mount\n"
"    -V, --version          show version information and exit\n\n"
	;
	static const char *version_str =
"fsdriver (FUSE driver for the Spring 2024 CS202 file system)\n"
"Written by Isami Romanowski.\n\n"
"Copyright (C) 2012, 2013 The University of Texas at Austin\n"
"This is free software; see the source or the provided COPYRIGHT file for\n"
"copying conditions.  There is NO warranty; not even for MERCHANTABILITY or\n"
"FITNESS FOR A PARTICULAR PURPOSE.\n\n"
	;

	switch (key) {
	case KEY_HELP:
		fputs(usage_str, stderr);
		fuse_opt_add_arg(outargs, "-ho");
		exit(fuse_main(outargs->argc, outargs->argv, &fs_oper, NULL));
	case KEY_VERSION:
		fputs(version_str, stderr);
		fuse_opt_add_arg(outargs, "--version");
		exit(fuse_main(outargs->argc, outargs->argv, &fs_oper, NULL));
	case KEY_TEST_OPS:
		if (loaded_imgname == NULL) {
			fprintf(stderr,
"fsdriver: need image for testing basic file system operations\n");
			exit(-1);
		} else {
			fs_test();
			exit(0);
		}
	default:
		return 1;
	}
}

int
main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
        // initializing imgname to be empty string, as opposed to NULL,
        // because the compiler is wrongly giving a warning
        // on the snprintf line below, and it could be scary.
        // This alternative avoids that warning.
	const char *imgname = "", *mntpoint = NULL;
	char fsname_buf[17 + PATH_MAX];
	int r;

	fuse_opt_add_arg(&args, argv[0]);
	if (argc < 2)
		panic("missing image or mountpoint parameter, see help");
	for (r = 1; r < argc; r++) {
		if (strlen(imgname) == 0 && argv[r][0] != '-' && strcmp(argv[r - 1], "-o") != 0) {
			imgname = argv[r];
		} else if(mntpoint == NULL && argv[r][0] != '-' && strcmp(argv[r - 1], "-o") != 0) {
			mntpoint = argv[r];
			fuse_opt_add_arg(&args, argv[r]);
		} else {
			fuse_opt_add_arg(&args, argv[r]);
		}
	}
        if (strlen(imgname) == 0)
            panic("No imgname");
        
	// Use a fsname (which shows up in df) in the style of sshfs, another
	// FUSE-based file system, with format "fsname#fslocation".
	snprintf(fsname_buf, sizeof(fsname_buf), "-ofsname=CS202fs#%s", imgname);
	fuse_opt_add_arg(&args, "-s"); // Always run single-threaded.
	fuse_opt_add_arg(&args, "-odefault_permissions"); // Kernel handles access.
	fuse_opt_add_arg(&args, fsname_buf); // Set the filesystem name.

	if (imgname == NULL) {
		fuse_opt_parse(&args, NULL, fs_opts, fs_parse_opt);
		return -1;
	} else {
		struct inode *dirroot;

		map_disk_image(imgname, mntpoint);

		// Make sure the superblock fields are sane.
		assert(super->s_magic == FS_MAGIC);
		assert(super->s_root != 0);

		// Guarantee that the root directory has proper permissions.
		// This is vital so that we can unmount the disk.
		dirroot = diskblock2memaddr(super->s_root);
		dirroot->i_mode = S_IFDIR | 0777;

		fuse_opt_parse(&args, NULL, fs_opts, fs_parse_opt);
		return fuse_main(args.argc, args.argv, &fs_oper, NULL);
	}
}
