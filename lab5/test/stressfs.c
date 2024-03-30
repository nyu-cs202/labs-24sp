#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <errno.h>

#include "../fs_types.h"
#include "../passert.h"

#define MINSTRESSBLKS	1050	// Guesstimated minimum number of blocks
				// needed to put file system through its paces
#define MAXSTATBLKS	5	// Maximum number of blocks left in the file
				// system for it to be considered "empty"
#define NFILES		512	// Number of files to create in phase 3

// Comment out any number of the following to disable running the
// specified test.
#define DO_PHASE1
#define DO_PHASE2
#define DO_PHASE3
#define DO_PHASE4

static char bigbuf[BLKSIZE], grossbuf[BLKSIZE];

void
_panic(int lineno, const char *file, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "\e[31mpanic at %s:%d\e[m: ", file, lineno);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);

	exit(-1);
}
#define panic(FMT, ...) _panic(__LINE__, __FILE__, FMT, ## __VA_ARGS__)

static inline void
mnt_statfs(struct statfs *stfs)
{
	if (statfs("mnt", stfs) < 0)
		panic("statfs mnt: %s", strerror(errno));
}

int
writen(int fd, char *buf, int size)
{
	off_t offset;
	int remaining;
	int r;

	offset = 0, remaining = size;
	while (remaining > 0) {
		r = write(fd, buf + offset, remaining);
		if (r < 0)
			return r;
		remaining -= r;
		offset += r;
	}
	return size;
}

void
report_statfs(struct statfs *stfs)
{
	int r;

	mnt_statfs(stfs);
	printf("statfs reports %lu/%lu blocks free on the file system\n", stfs->f_bfree, stfs->f_blocks);
}

void
phase4_rmdir(char *buf, char dname, int p)
{
	int r;

	// End case.
	if (dname > 'z')
		return;

	// Append new directory.
	buf[p++] = '/';
	buf[p++] = dname;
	buf[p] = '\0';

	// Depth-first.
	phase4_rmdir(buf, dname + 1, p);

	// Zero-terminate again, remove.
	buf[p] = '\0';
	printf("\tremoved %s\n", buf);
	if ((r = rmdir(buf)) < 0)
		panic("rmdir %s: %e", buf, r);
}

int
main(int argc, char **argv)
{
	struct statfs stfs;
	struct stat st;
	int fd, r, i;
	uint32_t before_bfree, wcount, rcount, before_rootsize;

	printf("stressfs running as environment %08x\n", getpid());
	memset(bigbuf, 0x5a, BLKSIZE);

	// Get a snapshot of current file system utilization.
	report_statfs(&stfs);
	before_bfree = stfs.f_bfree;
	if (stfs.f_bfree < MINSTRESSBLKS)
		panic("only %u blocks free; needs at least %u for stressfs! free up space on the image and try again", stfs.f_bfree, MINSTRESSBLKS);

#ifdef DO_PHASE1
	// Phase 1:
	// Create a monstrous file that fills up the rest of the file
	// system.  Make sure that we read back all the blocks we wrote
	// to the file properly, and that all file metadata is freed
	// properly when done.  This should test the correctness of the
	// double-indirect block handling in allocation, reading and freeing.
	printf("phase 1 (blockzilla, destroyer of free space)\n");
	if ((fd = open("mnt/blockzilla", O_CREAT | O_WRONLY, 0600)) < 0)
		panic("open /blockzilla: %s", strerror(errno));
	wcount = 0;
	do {
		((uint32_t *)bigbuf)[0] = wcount;
		if ((r = writen(fd, bigbuf, BLKSIZE)) < 0) {
			if (errno != ENOSPC && errno != EIO)
				panic("write failed before disk full: %s", strerror(errno));
			else
				break;
		}
		wcount++;
		if ((wcount % 128) == 0)
			printf("\twrote block %u of blockzilla\n", wcount);
	} while (1);
	printf("\tflushing blockzilla\n");
	close(fd);
	printf("\twriting to blockzilla is done\n");

	mnt_statfs(&stfs);
	if (stfs.f_bfree > MAXSTATBLKS)
		panic("write said disk was full, statfs says it isn't! %u blocks free, should be less than or equal to %u", stfs.f_bfree, MAXSTATBLKS);
	if ((fd = open("mnt/blockzilla", O_RDONLY)) < 0)
		panic("open /blockzilla: %s", strerror(errno));
	printf("\treading back %u blocks to check for consistency\n", wcount);
	rcount = 0;
	while (rcount < wcount) {
		((uint32_t *)bigbuf)[0] = rcount;
		if ((r = read(fd, grossbuf, BLKSIZE)) != BLKSIZE)
			break;
		if (memcmp(bigbuf, grossbuf, BLKSIZE) != 0)
			panic("block %u not read correctly", rcount);
		rcount++;
		if ((rcount % 128) == 0)
			printf("\tread block %u of blockzilla\n", rcount);
	}
	if (rcount != wcount)
		panic("# blocks read is not the same as # blocks written!");
	printf("\tflushing blockzilla\n");
	close(fd);
	printf("\treading from blockzilla is done\n");

	if ((r = remove("mnt/blockzilla")) < 0)
		panic("remove /blockzilla: %s", strerror(errno));
	mnt_statfs(&stfs);
	if (before_bfree != stfs.f_bfree)
		panic("disk block leak! %u blocks free before blockzilla, only %u blocks remain in its wake", before_bfree, stfs.f_bfree);
	sync();
	printf("phase 1 OK!\n");
#endif

	report_statfs(&stfs);

#ifdef DO_PHASE2
	// Phase 2:
	// Create a file.  Make sure its fields aren't a jumbled mess. Then
	// extend it and try to read from it, without writing anything to it.
	// Since the read function returns zeroes if a block hasn't been
	// allocated in a file (and no blocks should be allocated!), we should
	// get a bunch of zero blocks.  This should test if blocks are being
	// properly zeroed in the file system implementation.
	printf("phase 2 (attention to (zero) detail)\n");
	if ((fd = open("mnt/dummy", O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
		panic("open /dummy: %e", fd);
	if ((r = fstat(fd, &st)) < 0)
		panic("stat /dummy: %e", r);
	printf("\tverifying correctly initialized fields\n");
	assert(st.st_size == 0);
	assert(st.st_blocks == 0);
	assert((st.st_mode & S_IFMT) == S_IFREG);
	assert(st.st_nlink == 1);

	printf("\tchecking sparse file handling\n");
	if ((r = ftruncate(fd, stfs.f_blocks * BLKSIZE)) < 0)
		panic("ftruncate /dummy: %e", r);
	memset(bigbuf, 0, BLKSIZE);
	rcount = 0;
	while ((r = read(fd, grossbuf, BLKSIZE)) == BLKSIZE) {
		if (memcmp(bigbuf, grossbuf, BLKSIZE) != 0)
			panic("block %u didn't read back zero", rcount);
		rcount++;
	}
	close(fd);
	if (r < 0)
		panic("error in read /dummy: %s", strerror(errno));
	if (r > 0)
		panic("short read from /dummy: %u instead of %u", r, BLKSIZE);
	if ((r = remove("mnt/dummy")) < 0)
		panic("remove /dummy: %s", strerror(errno));
	printf("phase 2 OK!\n");
#endif

	report_statfs(&stfs);

#ifdef DO_PHASE3
	// Phase 3:
	// Create a large number of files in the root directory.  Write a
	// special block to each of them.  Then, make sure we can read the
	// directory entry of every file we made, and that the file data in
	// each file is consistent.  This should test that directories are
	// handled properly in file creation.
	printf("phase 3 (flight of the file creates)\n");
	before_bfree = stfs.f_bfree;
	if ((r = stat("mnt", &st)) < 0)
		panic("stat /: %s", strerror(errno));
	before_rootsize = st.st_blocks;
	printf("\tdirectory / has size of %lu blocks\n", st.st_blocks);

	printf("\tcreating %u files\n", NFILES);
	for (i = 0; i < NFILES; i++) {
		snprintf(bigbuf, BLKSIZE, "mnt/file%04u", i);
		if ((fd = open(bigbuf, O_CREAT | O_WRONLY, 0600)) < 0)
			panic("open %s: %s", bigbuf, strerror(errno));
		if ((r = writen(fd, bigbuf, BLKSIZE)) != BLKSIZE)
			panic("write %s: %s", bigbuf, strerror(errno));
		if ((i + 1) % 16 == 0)
			printf("\tcreated file %u\n", i);
		close(fd);
	}
	printf("\tsycing file system\n");
	sync();

	printf("\tverifying %u created files\n", NFILES);
	for (i = 0; i < NFILES; i++) {
		snprintf(bigbuf, BLKSIZE, "mnt/file%04u", i);
		if ((fd = open(bigbuf, O_RDONLY)) < 0)
			panic("open %s: %s", bigbuf, strerror(errno));
		if ((r = read(fd, grossbuf, BLKSIZE)) != BLKSIZE)
			panic("read %s: %s", bigbuf, strerror(errno));
		if (strcmp(bigbuf, grossbuf) != 0)
			panic("read from %s returned bad data", bigbuf);
		if ((i + 1) % 16 == 0)
			printf("\tverified file %u\n", i);
		close(fd);
	}

	printf("\tremoving %u created files\n", NFILES);
	for (i = 0; i < NFILES; ++i) {
		snprintf(bigbuf, BLKSIZE, "mnt/file%04u", i);
		if ((r = remove(bigbuf)) < 0)
			panic("remove %s: %s", bigbuf, strerror(errno));
		if ((i + 1) % 16 == 0)
			printf("\tremoved file %u\n", i);
	}
	printf("\tsyncing file system\n");
	sync();

	mnt_statfs(&stfs);
	if ((r = stat("mnt", &st)) < 0)
		panic("stat /: %e", r);
	printf("\tdirectory / now has size of %lu blocks\n", st.st_blocks);
	printf("phase 3 OK!\n");
#endif

	report_statfs(&stfs);

#ifdef DO_PHASE4
	// Phase 4:
	// Create a nested directory structure, and put at file at the leaf
	// nodes in the directory structure.  Make sure that all the file we
	// place is read back correctly.  This should test correct operation
	// of directory creation and traversal.
	printf("phase 4 (make like a directory and leaf)\n");
	before_bfree = stfs.f_bfree;
	bigbuf[0] = 'm';
	bigbuf[1] = 'n';
	bigbuf[2] = 't';
	bigbuf[3] = '\0';
	r = 3;
	for (i = 'a'; i <= 'z'; i++) {
		bigbuf[r++] = '/';
		bigbuf[r++] = i;
		bigbuf[r] = '\0';
		if ((fd = mkdir(bigbuf, 0700)) < 0)
			panic("mkdir %s: %s", bigbuf, strerror(errno));
		else {
			printf("\tcreated %s\n", bigbuf);
			close(fd);
		}
	}
	strcat(bigbuf, "/file");
	printf("\tcreating %s\n", bigbuf);
	if ((fd = open(bigbuf, O_CREAT | O_WRONLY, 0600)) < 0)
		panic("open %s: %s", bigbuf, strerror(errno));
	if ((r = writen(fd, bigbuf, BLKSIZE)) < 0)
		panic("write %s: %s", bigbuf, strerror(errno));
	close(fd);
	printf("\tverifying %s\n", bigbuf);
	if ((fd = open(bigbuf, O_RDONLY)) < 0)
		panic("open %s: %s", bigbuf, strerror(errno));
	if ((r = read(fd, grossbuf, BLKSIZE)) != BLKSIZE)
		panic("read %s: %s", bigbuf, strerror(errno));
	if (strcmp(bigbuf, grossbuf) != 0)
		panic("read from %s returned bad data", bigbuf);
	close(fd);
	printf("\tremoving directory structure\n");
	if ((r = remove(bigbuf)) < 0)
		panic("remove %s: %s", bigbuf, strerror(errno));
	phase4_rmdir(bigbuf, 'a', 3);

	mnt_statfs(&stfs);
	if (stfs.f_bfree != before_bfree)
		panic("disk block leak: %u blocks before, %u blocks after", before_bfree, stfs.f_bfree);
	printf("phase 4 OK!\n");
#endif

	report_statfs(&stfs);

	// Phase 5:
	// Celebrate.
	printf("all stressfs tests pass\n");

	return 0;
}
