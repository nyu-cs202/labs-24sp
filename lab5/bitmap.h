#pragma once

#include <stdbool.h>
#include <stdint.h>

int	alloc_block(void);
bool	block_is_free(uint32_t blockno);
void	free_block(uint32_t blockno);
