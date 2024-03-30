#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// The maximum length of a single file name, including
// the null-terminator.
#undef NAME_MAX
#define NAME_MAX		124

// The maximum allowable path length to a file from
// the root directory, including the null-terminator.
#undef PATH_MAX
#define PATH_MAX		1024

// The size of a block in the file system.
#define BLKSIZE			4096

// How many bits are present in a block.
#define BLKBITSIZE		(BLKSIZE * 8)

// The number of blocks which are addressable from the direct
// block pointers, the indirect block, and the double-indirect
// block.
#define N_DIRECT		10
#define N_INDIRECT		(BLKSIZE / 4)
#define N_DOUBLE		((BLKSIZE / 4) * N_INDIRECT)

#define MAX_FILE_SIZE	((N_DIRECT + N_INDIRECT + N_DOUBLE) * BLKSIZE)

struct inode {
	uid_t		i_owner; // Owner of inode.
	gid_t		i_group; // Group membership of inode.
	mode_t		i_mode; // Permissions and type of inode.
	dev_t		i_rdev; // Device represented by inode, if any.
	uint16_t	i_nlink; // The number of hard links.
	int64_t		i_atime; // Access time (reads).
	int64_t		i_ctime; // Change time (chmod, chown).
	int64_t		i_mtime; // Modification time (writes).
	uint32_t	i_size; // The size of the inode in bytes.

	// Block pointers.
	// A block is allocated iff its value is != 0.
	uint32_t	i_direct[N_DIRECT]; // Direct blocks.
	uint32_t	i_indirect; // Indirect block.
	uint32_t	i_double; // Double-indirect block.
} __attribute__((packed));

struct dirent {
	uint32_t	d_inum; // Block number of the referenced inode.
	char		d_name[NAME_MAX]; // File name.
} __attribute__((packed));

// The number of struct dirents in a data block.
#define BLKDIRENTS		(BLKSIZE / sizeof(struct dirent))

// The magic number signifying a valid superblock.
#define FS_MAGIC		0xC5202F19

struct superblock {
	uint32_t	s_magic; // Magic number: FS_MAGIC.
	uint32_t	s_nblocks; // Total number of blocks on disk.
	uint32_t	s_root; // Inum of the root directory inode.
} __attribute__((packed));

// Efficient min and max operations
#define MIN(_a, _b) \
({ \
	    __typeof__(_a) __a = (_a); \
	    __typeof__(_b) __b = (_b); \
	    __a <= __b ? __a : __b; \
})

#define MAX(_a, _b)\
({ \
	__typeof__(_a) __a = (_a); \
	__typeof__(_b) __b = (_b); \
	__a >= __b ? __a : __b; \
})

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n) \
({ \
	uint32_t __a = (uint32_t) (a); \
	(__typeof__(a)) (__a - __a % (n)); \
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)\
({ \
	uint32_t __n = (uint32_t) (n); \
	(__typeof__(a)) (ROUNDDOWN((uint32_t) (a) + __n - 1, __n)); \
})
