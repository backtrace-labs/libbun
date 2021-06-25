#define _GNU_SOURCE
#include "bun/utils.h"

#include <unistd.h>
#include <sys/syscall.h>

#if defined(BUN_MEMFD_CREATE_AVAILABLE)
#include <sys/memfd.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if defined(BUN_MEMFD_CREATE_AVAILABLE)

int
bun_memfd_create(const char *name, unsigned int flags)
{

	return memfd_create(name, flags);
}
#else

static int
open_mkstemp(const char *name, unsigned int flags)
{
	char *filename = NULL;
	int fd = -1;
	(void) flags;

	fd = asprintf(&filename, "%s.XXXXXX", name);
	if (fd == -1)
		goto error;

	fd = mkstemp(filename);
	if (fd == -1)
		goto error;

	if (unlink(filename) == -1)
		goto error;

	free(filename);
	return fd;
error:
	if (fd != -1)
		close(fd);
	free(filename);
	return -1;
}

static int
open_real_file(const char *name, unsigned int flags)
{
	int fd = -1;

#if defined(O_TMPFILE)
	fd = open(name, O_TMPFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
#else
	fd = open(name, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
#endif /* defined(O_TMPFILE) */
	if (fd < 0)
		return -1;

	if (unlink(name) == -1)
		return -1;

	return fd;
}

int
bun_memfd_create(const char *name, unsigned int flags)
{
	int fd = syscall(SYS_memfd_create, name, flags);

	if (fd == -1)
		fd = open_mkstemp(name, flags);

	if (fd == -1)
		fd = open_real_file(name, flags);

	return fd;
}

#endif /* BUN_MEMFD_CREATE_AVAILABLE */

pid_t
bun_gettid()
{

#if !defined(__ANDROID__)
	return (pid_t)syscall(SYS_gettid);
#else
	return gettid();
#endif
}
