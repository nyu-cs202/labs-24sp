#include <fuse.h>
#include <fuse_lowlevel.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "disk_map.h"
#include "panic.h"

void
_panic(int lineno, const char *file, const char *fmt, ...)
{
	struct fuse_context *context;
	struct fuse_session *session;
	struct fuse_chan *chan;
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "\x1B[31mpanic at %s:%d\x1B[m: ", file, lineno);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);

	context = fuse_get_context();
	if (context->fuse != NULL) {
		// Exit main event loop.
		fuse_exit(context->fuse);

		// Force unmount.  There should just be one channel associated
		// with the current FUSE session.
		session = fuse_get_session(context->fuse);
		chan = fuse_session_next_chan(session, NULL);
		fuse_unmount(loaded_mntpoint, chan);
	}
	exit(-1);
}
