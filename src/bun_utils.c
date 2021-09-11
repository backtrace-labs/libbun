#define _GNU_SOURCE
#include "bun/utils.h"

#include <unistd.h>
#include <sys/syscall.h>

#if defined(BUN_MEMFD_CREATE_AVAILABLE)
#include <sys/memfd.h>
#else
#include <stdint.h>
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
#include <sys/user.h>
#include <sys/ptrace.h>
#include <linux/elf.h>

#ifndef BUN_NO_BACKTRACE_LOG
#ifdef __ANDROID__
#include <android/log.h>
#define BT_LOG_DEBUG ANDROID_LOG_DEBUG
#define BT_LOG_ERROR ANDROID_LOG_ERROR
#define bt_log(LOG_LEVEL, ...) __android_log_print(LOG_LEVEL, "Backtrace-Android", __VA_ARGS__)
#else
#include <stdio.h>
#define BT_LOG_DEBUG 0
#define BT_LOG_ERROR 0
#define bt_log(LOG_LEVEL, ...) fprintf(stderr, __VA_ARGS__)
#endif
#else
#define bt_log(...)
#endif

static const char *bun_internal_cache_dir = NULL;

void
bun_cache_dir_set(const char *path)
{

	bun_internal_cache_dir = path;
	return;
}

const char *
bun_cache_dir_get()
{

	return bun_internal_cache_dir;
}

#if defined(BUN_MEMFD_CREATE_AVAILABLE)

int
bun_memfd_create(const char *name)
{

	return memfd_create(name, 0);
}
#else

static const char *
tmp_path()
{

	if (bun_cache_dir_get() != NULL) {
		return bun_cache_dir_get();
	} else {
#ifdef __ANDROID__
		return "/data/local/tmp";
#else
		return "/tmp";
#endif
	}
}

static int
open_mkstemp(const char *name)
{
	char *filename = NULL;
	int fd = -1;

	fd = asprintf(&filename, "%s/%s.XXXXXX", tmp_path(), name);
	if (fd == -1)
		goto error;

	// Android NDK does not support mkostemp prior to SDK version 23
	fd = mkstemp(filename);

	if (fd == -1)
		goto error;

	if (fcntl(fd, F_SETFD, O_CLOEXEC) == -1)
		goto error;

	if (unlink(filename) == -1)
		goto error;

	free(filename);
	return fd;
error:

	bt_log(ANDROID_LOG_DEBUG, "open_mkstemp() failed. errno: %d (%s)",
	    errno, strerror(errno));
	if (fd != -1)
		close(fd);
	free(filename);
	return -1;
}

static int
open_real_file(const char *name)
{
	char *final_name = NULL;
	int fd = -1;

	if (asprintf(&final_name, "%s/", bun_cache_dir_get()) == -1)
		goto error;

#if defined(O_TMPFILE)
	fd = open(final_name, O_TMPFILE | O_CLOEXEC | O_RDWR,
	    S_IRUSR | S_IWUSR);
	if (fd < 0)
		goto error;
#else
	fd = open(final_name, O_CREAT | O_CLOEXEC | O_TRUNC | O_RDWR,
	    S_IRUSR | S_IWUSR);
	if (fd < 0)
		goto error;
	if (unlink(final_name) == -1)
		goto error;
#endif /* defined(O_TMPFILE) */

	free(final_name);
	return fd;
error:

	bt_log(ANDROID_LOG_DEBUG, "open_real_file() failed. errno: %d (%s)",
	    errno, strerror(errno));
	if (fd != -1)
		close(fd);
	free(final_name);
	return -1;
}

int
bun_memfd_create(const char *name)
{
#if defined(SYS_memfd_create)
	int fd = syscall(SYS_memfd_create, name, 0);
#else
	int fd = -1;
#endif

	if (fd == -1)
		fd = open_mkstemp(name);

	if (fd == -1)
		fd = open_real_file(name);

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

#if defined(__aarch64__) || defined(__arm__)
#define MAX_USER_REGS_SIZE 650

static int
getregs_success(pid_t pid)
{
	uint64_t buffer[MAX_USER_REGS_SIZE / sizeof(uint64_t)];
	struct iovec io;
	int r;

	io.iov_base = buffer;
	io.iov_len = MAX_USER_REGS_SIZE * sizeof(uint64_t);

	r = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &io);
	bt_log(BT_LOG_ERROR, "PTRACE_GETREGS: %d", r);
	return r;
}
#else
static int
getregs_success(pid_t pid)
{
	struct user_regs_struct regs;
	int r;

	r = ptrace(PTRACE_GETREGS, pid, &regs, &regs);
	bt_log(BT_LOG_DEBUG, "PTRACE_GETREGS: %d", r);
	return r;
}
#endif

int
bun_waitpid(pid_t pid, int msec_timeout)
{
	struct timespec begin;
	struct timespec current, remaining;
	int loop = 0;

	for (;;) {
		int r, sig;
		int status, e;
		const int flags = WNOHANG | WUNTRACED;
		siginfo_t sinfo;

		pid_t p = waitpid(pid, &status, flags);
		if (p == -1) {
			bt_log(BT_LOG_DEBUG, "Observed -1 in waitpid: %d\n", errno);
			if (errno == EINTR)
				continue;

			if (errno == ECHILD) {
				bt_log(BT_LOG_DEBUG, "Received child stop "
									 "notification; retrying\n");
				continue;
			}

			return -1;
		} else if (p != pid) {
			bt_log(BT_LOG_DEBUG, "No matched event: %d != %ju\n", p,
				   (uintmax_t) pid);

			/* No status to report, try again. */
			current.tv_sec = 0;
			current.tv_nsec = 500000;

			for (;;) {
				r = nanosleep(&current, &remaining);
				if (r != -1 || errno != EINTR)
					break;

				current = remaining;
			}

			msec_timeout -= loop++ & 1;
			if (msec_timeout > 0) {
				bt_log(BT_LOG_DEBUG, "Trying again, timeout is: %d\n", msec_timeout);
				continue;
			}

			/*
			* There are two reasons why we may have reached this
			* timeout:
			*
			* 1) The process was already stopped when we attached
			*    and we did not observe a state transition.
			* 2) The process was running when we attached but it
			*    has not yet transitioned to a stop state.  This
			*    case is extremely unlikely unless the timeout
			*    period was very short.
			*
			* In either case, we may as well try to get register
			* values before giving up.  If the process truly is not
			* stopped yet then this call will fail and we can
			* indicate failure.  However, if it succeeds then we
			* probably encountered the first case above and it is
			* safe for us to proceed.
			*/

			if (getregs_success(pid) != -1)
				goto stopped;

			return -1;
		}

		/* The child has terminated. */
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			bt_log(BT_LOG_ERROR, "process already exited with code %d",
			    WEXITSTATUS(status));
			return -1;
		}

		if (WIFSTOPPED(status) == false) {
			bt_log(BT_LOG_ERROR, "process stopped with unexpected status %d",
			    status);
			return -1;
		}

		sig = WSTOPSIG(status);
		bt_log(BT_LOG_DEBUG, "Process %ju stopped with signal %d\n",
		    (uintmax_t) pid, sig);

		/*
		 * From the Linux ptrace(2) man page:
		* If WSTOPSIG(status) is not one of SIGSTOP, SIGTSTP,
		* SIGTTIN, or SIGTTOU, then this can't be a group-stop.
		* If it is one of these signals, then we must use
		* PTRACE_GETSIGINFO.  If that fails with EINVAL then we
		* are definitely in a group-stop.  Otherwise we are in
		* a signal-delivery-stop.  We already know that this
		* signal is a SIGSTOP because otherwise we would have
		* returned above.
		*
		* Being in group-stop means that we are currently
		* handling a signal sent from another source (i.e. not
		* from our PTRACE_ATTACH), so we should store the
		* signal for injection when detaching.
		*/
		switch (sig) {
			case SIGSTOP:
			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				break;
			default:
				goto signal_stop;
		}

		r = ptrace(PTRACE_GETSIGINFO, pid, NULL, &sinfo);
		if (r != -1)
			goto signal_stop;

		e = errno;
		bt_log(BT_LOG_DEBUG, "Failed to retrieve siginfo for "
		    "process %ju: %s\n", (uintmax_t) pid, strerror(e));

		switch (e) {
		case EINVAL:
			break;
		case ESRCH:
			bt_log(BT_LOG_DEBUG, "Process %ju was killed "
			    "from under us\n", (uintmax_t) pid);
			return -1;
		default:
			bt_log(BT_LOG_DEBUG, "Failed to read signal "
			    "information from process %ju: %s\n",
			    (uintmax_t) pid, strerror(e));
			goto stopped;
	}

		bt_log(BT_LOG_DEBUG, "Process %ju is in group-stop "
		    "state; re-injecting SIGSTOP\n", (uintmax_t) pid);

		goto stopped;
signal_stop:
		goto stopped;
	}

stopped:
	return 0;
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
