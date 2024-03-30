#pragma once

#include "fs_types.h"

extern uint32_t			*bitmap;
extern struct superblock	*super;
extern struct stat		 diskstat;
extern uint8_t			*diskmap;
extern const char		*loaded_imgname;
extern const char		*loaded_mntpoint;

void	*diskblock2memaddr(uint32_t blockno);
void	 flush_block(void *addr);
void	 map_disk_image(const char *imgname, const char *mntpoint);
