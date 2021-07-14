#include "gtest/gtest.h"

#include <atomic>
#include <algorithm>
#include <functional>
#include <vector>
#include <string>

#include <fcntl.h>

#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "bun/bun.h"
#include "bun/stream.h"
#include "bun/utils.h"

#include "bcd.h"


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

	fd = bun_memfd_create("_backtrace_buffer", MFD_CLOEXEC);
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
	time_t now = time(NULL);

	bun_handle handle;
	bool bun_initialized = bun_handle_init(&handle, BUN_BACKEND_LIBUNWIND);
	if (!bun_initialized) {
		return -1;
	}

	bun_buffer buf = { buffer_child, BUFFER_SIZE };

	auto written = bun_unwind_remote(&handle, &buf, tid);

	return -1;
}

static void
monitor_init(void)
{
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

	bcd_emit(&bcd, "1");

	return false;
}

TEST(libunwind_poison_malloc_remote, unwinding)
{
	struct bcd_config cf;
	bcd_error_t e;

	/* Initialize a shared memory region. */
	buffer = static_cast<char *>(buffer_create());

	/* Initialize the BCD configuration file. */
	if (bcd_config_init(&cf, &e) == -1)
		FAIL();

	/* Request handler to be called when processing errors by BCD worker. */
	cf.request_handler = request_handler;

	/* Set a function to be called by the child for setting permissions. */
	cf.monitor_init = monitor_init;

	if (bcd_init(&cf, &e) == -1) {
		char buf[512];
		sprintf(buf, "error: failed to init: %s (%s)\n",
		    e.message, strerror(e.errnum));
		FAIL() << buf;
	}

	/* Initialize the BCD handler. */
	if (bcd_attach(&bcd, &e) == -1)
		FAIL();

	buf.data = (char *)buffer;
	buf.size = BUFFER_SIZE;

	pid_t child_pid;
	memcpy(&child_pid, buffer, sizeof(child_pid));

	prctl(PR_SET_PTRACER, child_pid, 0, 0, 0);
	prctl(PR_SET_DUMPABLE, 1);

	raise(SIGPWR);
	bcd_emit(&bcd, "AnyText");
	raise(SIGPWR);

	bcd_detach(&bcd, &e);
}
