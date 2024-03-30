#pragma once

void	_panic(int lineno, const char *file, const char *fmt, ...);

#define panic(FMT, ...) _panic(__LINE__, __FILE__, FMT, ## __VA_ARGS__)
