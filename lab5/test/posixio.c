#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../passert.h"
#include "../fs_types.h"

const char *msg = "This is a rather uninteresting message.\n\n";

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
#define panic(FMT, ...) _panic(__LINE__, __FILE__, FMT, ## __VA_ARGS__)

int
main(int argc, char **argv)
{
	int r, f, fd, i;
	struct stat st;
	char buf[512];

	if ((r = open("mnt/not-found", O_RDONLY)) < 0 && errno != ENOENT)
		panic("open /not-found: %s", strerror(errno));
	else if (r >= 0)
		panic("open /not-found succeeded!");

	if ((fd = open("mnt/msg", O_RDONLY)) < 0)
		panic("open /msg: %s", strerror(errno));
	printf("open is good\n");

	if ((r = fstat(fd, &st)) < 0)
		panic("inode_stat: %s", strerror(errno));
	if (strlen(msg) != st.st_size)
		panic("inode_stat returned size %d wanted %d\n", st.st_size, strlen(msg));
	printf("inode_stat is good\n");

	memset(buf, 0, sizeof buf);
	if ((r = read(fd, buf, sizeof buf)) < 0)
		panic("inode_read: %s", strerror(errno));
	if (strcmp(buf, msg) != 0)
		panic("inode_read returned wrong data");
	printf("inode_read is good\n");

	if ((r = close(fd)) < 0)
		panic("inode_close: %s", r);
	printf("inode_close is good\n");

	// Try writing
	if ((fd = open("mnt/new-file", O_RDWR|O_CREAT, 0600)) < 0)
		panic("open /new-file: %s", strerror(errno));

	if ((r = write(fd, msg, strlen(msg))) != strlen(msg))
		panic("inode_write: %s", strerror(errno));
	printf("inode_write is good\n");

	lseek(fd, 0, SEEK_SET);
	memset(buf, 0, sizeof buf);
	if ((r = read(fd, buf, sizeof buf)) < 0)
		panic("inode_read after inode_write: %s", strerror(errno));
	if (r != strlen(msg))
		panic("inode_read after inode_write returned wrong length: %d", r);
	if (strcmp(buf, msg) != 0)
		panic("inode_read after inode_write returned wrong data");
	printf("inode_read after inode_write is good\n");

	// Try files with indirect blocks
	if ((f = open("mnt/big", O_WRONLY|O_CREAT, 0600)) < 0)
		panic("creat /big: %s", strerror(errno));
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < (N_DIRECT*3)*BLKSIZE; i += sizeof(buf)) {
		*(int*)buf = i;
		if ((r = write(f, buf, sizeof(buf))) < 0)
			panic("write /big@%d: %s", i, strerror(errno));
	}
	close(f);

	if ((f = open("mnt/big", O_RDONLY)) < 0)
		panic("open /big: %s", strerror(errno));
	for (i = 0; i < (N_DIRECT*3)*BLKSIZE; i += sizeof(buf)) {
		*(int*)buf = i;
		if ((r = read(f, buf, sizeof(buf))) < 0)
			panic("read /big@%d: %s", i, strerror(errno));
		if (r != sizeof(buf))
			panic("read /big from %d returned %d < %d bytes",
			      i, r, sizeof(buf));
		if (*(int*)buf != i)
			panic("read /big from %d returned bad data %d",
			      i, *(int*)buf);
	}
	close(f);
	printf("large file is good\n");

	return 0;
}
