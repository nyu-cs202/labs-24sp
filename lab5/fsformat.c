#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "passert.h"
#include "fs_types.h"

#define MAX_DIR_ENTS 128
#define START_DIR_ENTS MAX_DIR_ENTS

struct IDir {
	struct inode *inode;
	struct dirent *ents;
	uint32_t n;
	uint32_t capacity;
};

uint32_t nblocks;
char *diskmap, *diskpos;
struct superblock *super;
uint32_t *bitmap;

static time_t curtime;
static uid_t curuid;
static gid_t curgid;

// Dummy var to placate panic.
const char *loaded_mntpoint;

void
_panic(int lineno, const char *file, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "\x1B[31mpanic at %s:%d\x1B[m: ", file, lineno);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);

	exit(-1);
}

void
readn(int f, void *out, size_t n)
{
	size_t p = 0;
	while (p < n) {
		size_t m = read(f, out + p, n - p);
		if (m < 0)
			panic("read: %s", strerror(errno));
		if (m == 0)
			panic("read: Unexpected EOF");
		p += m;
	}
}

uint32_t
blockof(void *pos)
{
	return ((char*)pos - diskmap) / BLKSIZE;
}

void *
alloc(uint32_t bytes)
{
	void *start = diskpos;
	diskpos += ROUNDUP(bytes, BLKSIZE);
	if (blockof(diskpos) >= nblocks)
		panic("out of disk blocks");
	return start;
}

void
opendisk(const char *name, struct IDir *iroot)
{
	int r, diskfd, nbitblocks;

	if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0)
		panic("open %s: %s", name, strerror(errno));

	if ((r = ftruncate(diskfd, 0)) < 0
	    || (r = ftruncate(diskfd, nblocks * BLKSIZE)) < 0)
		panic("truncate %s: %s", name, strerror(errno));

	if ((diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ|PROT_WRITE,
			    MAP_SHARED, diskfd, 0)) == MAP_FAILED)
		panic("mmap %s: %s", name, strerror(errno));

	close(diskfd);

	diskpos = diskmap;
	super = alloc(BLKSIZE);

	nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
	bitmap = alloc(nbitblocks * BLKSIZE);
	memset(bitmap, 0xFF, nbitblocks * BLKSIZE);

	iroot->inode = alloc(BLKSIZE);
	iroot->inode->i_mode = S_IFDIR | 0777;
	iroot->inode->i_nlink = 1;
	iroot->inode->i_atime = curtime;
	iroot->inode->i_ctime = curtime; 
	iroot->inode->i_mtime = curtime;
	iroot->inode->i_owner = curuid;
	iroot->inode->i_group = curgid;

	super->s_magic = FS_MAGIC;
	super->s_nblocks = nblocks;
	super->s_root = blockof(iroot->inode);
}

void
finishdisk(void)
{
	int r, i;

	for (i = 0; i < blockof(diskpos); ++i)
		bitmap[i/32] &= ~(1<<(i%32));

	if ((r = msync(diskmap, nblocks * BLKSIZE, MS_SYNC)) < 0)
		panic("msync: %s", strerror(errno));
}

void
finishinode(struct inode *inode, uint32_t start, uint32_t len)
{
	int i, j;
	inode->i_size = len;
	len = ROUNDUP(len, BLKSIZE);

	// Write direct blocks.
	for(i = 0; i < len / BLKSIZE && i < N_DIRECT; ++i)
		inode->i_direct[i] = start + i;

	if(i == N_DIRECT) {
		// Write indirect blocks.
		uint32_t *ind = alloc(BLKSIZE);
		inode->i_indirect = blockof(ind);
		for(; i < len / BLKSIZE && i < N_DIRECT + N_INDIRECT; ++i)
			ind[i - N_DIRECT] = start + i;

		if(i == N_DIRECT + N_INDIRECT) {
			// Write doubly-indirect (inindirect) blocks.
			uint32_t *inind = alloc(BLKSIZE);
			inode->i_double = blockof(inind);
			j = 0;
			while(j < N_INDIRECT) {
				// Allocate indirect block.
				ind = alloc(BLKSIZE);
				inind[j] = blockof(ind);

				// Load indirect block.
				for(; i < len / BLKSIZE &&
					    i < N_DIRECT + N_INDIRECT +
					    (j + 1) * N_INDIRECT; ++i)
					ind[i - N_DIRECT - N_INDIRECT -
					    j * N_INDIRECT] = start + i;

				// Advance inindirect pointer.
				j++;
			}
		}
	}
}

void
startidir(struct IDir *idout)
{
	idout->ents = malloc(START_DIR_ENTS * sizeof(struct dirent));
	idout->n = 0;
	idout->capacity = START_DIR_ENTS;
}

struct inode *
idiradd(struct IDir *id, uint32_t mode, const char *name)
{
	struct dirent *out = &id->ents[id->n++], *p;
	struct inode *iout;

	if(id->n >= id->capacity) {
		// Increase capacity.
		if((id->capacity << 2) < id->capacity)
			id->capacity = -1u;
		else
			id->capacity <<= 2;

		if(id->n >= id->capacity)
			panic("too many directory entries");

		// Attempt to reallocate to match.
		if(!(p = realloc(id->ents,
				 id->capacity * sizeof(struct dirent))))
			panic("ran out of memory trying to add directory "
			      "entry");
		id->ents = p;
	}

	// Create inode for this directory entry.
	iout = alloc(BLKSIZE);
	iout->i_mode = mode;
	iout->i_size = 0;
	iout->i_nlink = 1;
	iout->i_atime = curtime;
	iout->i_ctime = curtime;
	iout->i_mtime = curtime;
	iout->i_owner = curuid;
	iout->i_group = curgid;

	// Copy name and inumber to directory entry.
	strcpy(out->d_name, name);
	out->d_inum = blockof(iout);

	return iout;
}

void
finishidir(struct IDir *id)
{
	uint32_t size = id->n * sizeof(struct dirent);
	void *start = alloc(size);
	memmove(start, id->ents, size);
	finishinode(id->inode, blockof(start), ROUNDUP(size, BLKSIZE));
	free(id->ents);
	id->ents = NULL;
}

void
writeinode(struct IDir *idir, const char *name)
{
	int r, fd;
	struct inode *inode;
	struct stat st;
	const char *last;
	char *start;

	if ((fd = open(name, O_RDONLY)) < 0)
		panic("open %s: %s", name, strerror(errno));
	if ((r = fstat(fd, &st)) < 0)
		panic("stat %s: %s", name, strerror(errno));
	if (!S_ISREG(st.st_mode))
		panic("%s is not a regular file", name);

	last = strrchr(name, '/');
	if (last)
		last++;
	else
		last = name;

	inode = idiradd(idir, S_IFREG | 0600, last);
	start = alloc(st.st_size);
	readn(fd, start, st.st_size);
	finishinode(inode, blockof(start), st.st_size);
	close(fd);
}

void
usage(void)
{
	fprintf(stderr, "usage: fsformat IMAGE NBLOCKS [FILE]...\n");
	exit(-1);
}

int
main(int argc, char **argv)
{
	int i;
	char *s;
	struct IDir iroot;

	if (optind + 2 > argc)
		usage();

	nblocks = strtol(argv[optind + 1], &s, 0);
	if (*s || s == argv[optind + 1] || nblocks < 2)
		usage();

	curtime = time(NULL);
	curuid = getuid();
	curgid = getgid();
	opendisk(argv[optind], &iroot);

	startidir(&iroot);
	for (i = optind + 2; i < argc; ++i)
		writeinode(&iroot, argv[i]);
	finishidir(&iroot);

	finishdisk();
	return 0;
}
