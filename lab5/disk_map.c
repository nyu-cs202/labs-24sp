#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "passert.h"
#include "panic.h"
#include "disk_map.h"

uint32_t		*bitmap;
struct superblock	*super;
struct stat		 diskstat;
uint8_t			*diskmap;
const char		*loaded_imgname;
const char		*loaded_mntpoint;

// Maps a block number to an address.  The pointer returned
// points to the first byte of the specified block in mapped memory.
void *
diskblock2memaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskblock2memaddr", blockno);
	return (char *)(diskmap + blockno * BLKSIZE);
}

// Schedules the disk block associated with the given address to be
// flushed to disk.
void
flush_block(void *addr)
{
	addr = (void *)((intptr_t)addr & ~(BLKSIZE - 1));
	if (msync(addr, BLKSIZE, MS_ASYNC) < 0)
		panic("msync(%p): %s", addr, strerror(errno));
}

void
map_disk_image(const char *imgname, const char *mntpoint)
{
	int r, fd;

	assert(imgname != NULL);

	if (loaded_imgname != NULL || loaded_mntpoint != NULL)
		panic("attempting to map a disk over an existing one!");

	if ((fd = open(imgname, O_RDWR)) < 0)
		panic("open(%s): %s", imgname, strerror(errno));
	if ((r = fstat(fd, &diskstat)) < 0)
		panic("fstat(%s): %s", imgname, strerror(errno));
	if ((diskmap = mmap(NULL, diskstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
		panic("mmap(%s): %s", imgname, strerror(errno));

	super = (struct superblock *)diskmap; // = diskmap(0)
	bitmap = diskblock2memaddr(1);

	loaded_imgname = imgname;
	loaded_mntpoint = mntpoint;
}
