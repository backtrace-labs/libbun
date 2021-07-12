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
#include <sys/user.h>
#include <sys/ptrace.h>

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
	struct timespec current, remaining;
	int loop = 0;

	clock_gettime(CLOCK_MONOTONIC, &begin);

	for (;;) {
		int r, sig;
		int status, e;
		const int flags = WNOHANG;
		siginfo_t sinfo;

		pid_t p = waitpid(pid, &status, flags);
		if (p == -1) {
			if (errno == EINTR)
				continue;

			if (errno == ECHILD) {
				// bt_log(BT_LOG_DEBUG, "Received child stop "
				// 	"notification; retrying\n");
				continue;
			}

			// return bt_error_set(error, "waitpid", errno);
			return -1;
		} else if (p != pid) {
			// struct user_regs_struct regs;

			// bt_log(BT_LOG_DEBUG, "No matched event: %d != %ju\n", p,
			// 	(uintmax_t)pid);

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
			if (msec_timeout > 0)
				continue;

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
			// r = ptrace(PTRACE_GETREGS, pid, &regs, &regs);
			// if (r != -1)
			// 	goto stopped;

			// return bt_error_set(error, "timed out", ms);
			return -1;
		}

		/* The child has terminated. */
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			// return bt_error_set(error, "process already "
			// 	"exited with code", WEXITSTATUS(status));
			return -1;
		}

		if (WIFSTOPPED(status) == false) {
			// return bt_error_set(error, "process stopped with "
			// 	"unexpected status", status);
			return -1;
		}

		sig = WSTOPSIG(status);
		// bt_log(BT_LOG_DEBUG, "Process %ju stopped with signal "
		// 	"%d\n", (uintmax_t)pid, sig);

		/*
		* We only want to store signals from the initial attach
		* to reinject when detaching.
		*/
		// if (live->flags & BT_TARGET_PROCESS_F_STOPPED)
		// 	goto stopped;

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
		// bt_log(BT_LOG_DEBUG, "Failed to retrieve siginfo for "
		// 	"process %ju: %s\n", (uintmax_t)pid,
		// 	bt_strerror(e));

		switch (e) {
		case EINVAL:
			break;
		case ESRCH:
			// bt_log(BT_LOG_DEBUG, "Process %ju was killed "
			// 	"from under us\n", (uintmax_t)pid);
			return -1;
		default:
			// bt_log(BT_LOG_DEBUG, "Failed to read signal "
			// 	"information from process %ju: %s\n",
			// 	(uintmax_t)pid, bt_strerror(e));
			goto stopped;
		}

		// bt_log(BT_LOG_DEBUG, "Process %ju is in group-stop "
		// 	"state; re-injecting SIGSTOP\n", (uintmax_t)pid);

		/*
			* Store the observed signal so that we can re-inject it
			* when detaching.
			*/
		// md->inject = sig;
		goto stopped;

signal_stop:
		/*
		* We should still reinject any observed signal other
		* than SIGSTOP when detaching because such a signal
		* must have come from an external source.
		*/
		// if (sig != SIGSTOP)
		// 	md->inject = sig;

		goto stopped;
	}


	// for (;;) {
	// 	struct timespec end;
	// 	pid_t waitpid_result;
	// 	int status;

	// 	waitpid_result = waitpid(pid, &status, WNOHANG | __WALL);
	// 	if (waitpid_result != pid) {
	// 		if (waitpid_result == 0 || errno == EINTR ||
	// 		    errno == ECHILD || errno == EAGAIN) {
	// 			long diff_ms;

	// 			clock_gettime(CLOCK_MONOTONIC, &end);

	// 			diff_ms = (end.tv_sec - begin.tv_sec) * 1000 +
	// 			    (end.tv_nsec - begin.tv_nsec) / 1000000;

	// 			if (diff_ms < msec_timeout)
	// 				continue;
	// 		}
	// 		return -1;
	// 	}

	// 	/* The child has already terminated. */
	// 	if (WIFEXITED(status) || WIFSIGNALED(status)) {
	// 		return -1;
	// 	}

	// 	/* Unexpected status. */
	// 	if (WIFSTOPPED(status) == false) {
	// 		return -1;
	// 	}

	// 	return 0;
	// }
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