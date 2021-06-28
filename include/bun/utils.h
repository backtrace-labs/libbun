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

#ifdef __cplusplus
}
#endif
