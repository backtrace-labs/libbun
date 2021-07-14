#pragma once
/*
 * Copyright (c) 2021 Backtrace I/O, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <signal.h>
#include <stdbool.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This function is a wrapper around memfd_create(), which might not be
 * available as a library function on some systems.
 */
int bun_memfd_create(const char *name, unsigned int flags);

/*
 * This function returns the current thread id.
 */
pid_t bun_gettid();

/*
 * This function performs demangling of C++ symbol names. The destination buffer
 * is only written to if the function succeeds and the demangled name can be
 * written into the destination buffer, including the terminating null
 * character.
 */
bool bun_unwind_demangle(char *dest, size_t dest_size, const char *src);

/*
 * This function waits for the pid to become signalled.
 *
 * Returns a negative number on failure and 0 on success.
 */
int bun_waitpid(pid_t pid, int msec_timeout);

/*
 * This is the definition of signal a handler function. It is congruent with
 * the extended signal handler used by sigaction(2) with SA_SIGINFO flag.
 *
 * The last parameter is ucontext_t *, but is kept as void * for consistency
 * with the above definition.
 */
typedef void (bun_signal_handler_fn)(int, siginfo_t *, void *);

/*
 * Registers the passed signal handler using sigaction(2) with the SA_SIGINFO
 * flag. The following signals are handled:
 * - SIGABRT
 * - SIGBUS
 * - SIGSEGV
 * - SIGILL
 * - SIGSYS
 * - SIGTRAP
 *
 * The old handlers are not preserved, if the user wants to save them, it's
 * their responsibility.
 *
 * Returns true upon succes.
 * Returns false upon failure. The signal handlers may have been partially
 * changed.
 */
bool bun_register_signal_handler(bun_signal_handler_fn *handler);

#ifdef __cplusplus
}
#endif
