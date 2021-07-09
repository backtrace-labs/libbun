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
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
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

int
bun_waitpid(pid_t pid, int msec_timeout)
{
	struct timespec begin;

	clock_gettime(CLOCK_MONOTONIC, &begin);
	for (;;) {
		struct timespec end;
		pid_t waitpid_result;
		int status;

		waitpid_result = waitpid(pid, &status, WNOHANG);
		if (waitpid_result != pid) {
			if (waitpid_result == 0 || errno == EINTR ||
			    errno == ECHILD || errno == EAGAIN) {
				long diff_ms;

				clock_gettime(CLOCK_MONOTONIC, &end);

				diff_ms = (end.tv_sec - begin.tv_sec) * 1000 +
				    (end.tv_nsec - begin.tv_nsec) / 1000000;

				if (diff_ms < msec_timeout)
					continue;
			}
			return -1;
		}

		/* The child has already terminated. */
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			return -1;
		}

		/* Unexpected status. */
		if (WIFSTOPPED(status) == false) {
			return -1;
		}

		return 0;
	}
}

bool
bun_register_signal_handler(bun_signal_handler_fn *handler)
{
	const int signals[] = {
	    SIGABRT, SIGBUS, SIGSEGV, SIGILL, SIGSYS, SIGTRAP
	};
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO;
	for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
		if (sigaction(signals[i], &action, NULL) != 0)
			return false;
	}

	return true;
}