#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <bun/utils.h>

#include "bcd.h"
#include "crashpad.hpp"


/* Default size of buffer. */
#define BUFFER_SIZE	65536

/* This descriptor is shared between the child and parent. */
static int buffer_fd;

/* This is the pointer to the buffer used by the BCD child. */
static const char *buffer;
static char *buffer_child;

static bcd_t bcd;
static struct bun_buffer buf;

static void *
buffer_reader(int fd, int flags)
{
	void *r;

	r = mmap(NULL, BUFFER_SIZE, flags, MAP_SHARED, fd, 0);
	if (r == MAP_FAILED)
		abort();

	return r;
}

static void *
buffer_create(void)
{
	void *r;
	int fd;

	fd = bun_memfd_create("_backtrace_buffer");
	if (fd == -1)
		abort();

	if (ftruncate(fd, BUFFER_SIZE) == -1)
		abort();

	buffer_fd = fd;

	r = buffer_reader(buffer_fd, PROT_READ|PROT_WRITE);

	return r;
}

static int
request_handler(pid_t tid)
{
	fprintf(stderr, "retquest tid: %d\n", tid);
	bun_handle handle;

	bool bun_initialized = bun_handle_init(&handle, BUN_BACKEND_DEFAULT);
	if (!bun_initialized) {
		return -1;
	}

	bun_buffer buf = { buffer_child, BUFFER_SIZE };

	auto written = bun_unwind_remote(&handle, &buf, tid);
	(void) written;

	return -1;
}

static void
monitor_init(void)
{
	fprintf(stderr, "child pid: %d\n", getpid());
	/*
	 * This is called after the parent process has set buffer_fd. Set
	 * a memory mapping to the same descriptor.
	 */
	buffer_child = (char *)buffer_reader(buffer_fd, PROT_READ | PROT_WRITE);
	if (buffer_child == NULL)
		abort();

	pid_t child_pid = getpid();
	memcpy(buffer_child, &child_pid, sizeof(child_pid));
	return;
}

/*
 * Signal handler executed by CrashpadClient::SetFirstChanceExceptionHandler.
 */
bool FirstChanceHandler(int signum, siginfo_t *info, ucontext_t *context)
{
	(void) signum;
	(void) info;
	(void) context;

	thread_local bool flag;

	if (flag == false) {
		flag = true;
		bcd_emit(&bcd, "1");

		backtrace::dumpWithoutCrash(context);

		return true;
	} else {
		return false;
	}

	return true;
}

int
main(void)
{
	struct bcd_config cf;
	bcd_error_t e;

	/* Initialize a shared memory region. */
	buffer = static_cast<char *>(buffer_create());

	/* Initialize the BCD configuration file. */
	if (bcd_config_init(&cf, &e) == -1)
		abort();

	/* Request handler to be called when processing errors by BCD worker. */
	cf.request_handler = request_handler;

	/* Set a function to be called by the child for setting permissions. */
	cf.monitor_init = monitor_init;

	if (bcd_init(&cf, &e) == -1) {
		abort();
	}

	/* Initialize the BCD handler. */
	if (bcd_attach(&bcd, &e) == -1)
		abort();

	buf.data = (char *)buffer;
	buf.size = BUFFER_SIZE;

	pid_t child_pid;
	memcpy(&child_pid, buffer, sizeof(child_pid));

	prctl(PR_SET_PTRACER, child_pid, 0, 0, 0);
	prctl(PR_SET_DUMPABLE, 1);

	if (!backtrace::initCrashpad(&buf, FirstChanceHandler)) {
		return EXIT_FAILURE;
	}

	backtrace::crash();

	bcd_detach(&bcd, &e);
	return 0;
}
