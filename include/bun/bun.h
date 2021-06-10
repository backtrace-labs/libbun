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
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * This library offers a common API over a set of popular unwinding libraries.
 */

/*
 * Enumeration used to select the unwinding backend library. The library must
 * be present on the system.
 */
#if !defined(BUN_DETECTED_SYSTEM_BACKEND)
#define BUN_DETECTED_SYSTEM_BACKEND BUN_BACKEND_NONE
#endif
enum bun_unwind_backend {
    BUN_BACKEND_NONE = -1,
#if defined(BUN_LIBUNWIND_ENABLED)
    BUN_BACKEND_LIBUNWIND = 0,
#endif /* BUN_LIBUNWIND_ENABLED */
#if defined(BUN_LIBBACKTRACE_ENABLED)
    BUN_BACKEND_LIBBACKTRACE = 1,
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
    BUN_BACKEND_LIBUNWINDSTACK = 2,
#endif /* BUN_LIBUNWINDSTACK_ENABLED */
    BUN_BACKEND_DEFAULT = BUN_DETECTED_SYSTEM_BACKEND
};

/*
 * Enumeration used to determine the target architecture. Currently only used
 * for informational purposes.
 *
 * The build script adds a preprocessor definition BUN_ARCH_DETECTED, which is
 * set to the current processor architecture.
 */
enum bun_architecture {
    BUN_ARCH_UNKNOWN,
    BUN_ARCH_X86,
    BUN_ARCH_X86_64,
    BUN_ARCH_ARM,
    BUN_ARCH_ARM64
};

struct bun_handle;
struct bun_buffer;
/*
 * The typedef for the unwind function.
 *
 * Parameters:
 * - handle
 * - output buffer pointer
 *
 * Returns:
 * - number of bytes written to the buffer.
 *
 * Return value of 0 indicates an error.
 */
typedef size_t (unwind_fn)(struct bun_handle *, struct bun_buffer *);

/*
 * The typedef for the handle destructor function. If necessary, it will
 * execute backend-specific code.
 */
typedef void (handle_destructor_fn)(struct bun_handle *);

/*
 * bun_handle is handle used by the library. It contains backend-specific data
 * and should not be modified by code outside of libbun.
 *
 * Client code should treat it as an opaque type.
 */
struct bun_handle
{
    unwind_fn *unwind;
    handle_destructor_fn *destroy;
    void *backend_context;
    uint64_t flags;
    int write_count;
};

enum bun_handle_flags {
    BUN_HANDLE_WRITE_ONCE = (1ULL << 0)
};

/*
 * Initializes the bun_handle for the specified backend.
 *
 * Returns true for success.
 */
bool bun_handle_init(struct bun_handle *handle, enum bun_unwind_backend backend);

/*
 * The de-initialization function of bun_handle.
 */
void bun_handle_deinit(struct bun_handle *);

/*
 * This function unwinds from the current context. The result is stored into the
 * passed buffer.
 *
 * Parameters:
 * - handle - libbun handle
 * - buffer - pointer to the output buffer
 *
 * Returns the number of bytes written.
 *
 * This function is safe to use from signal handlers (for backends that allow
 * signal-safe unwinding).
 */
size_t bun_unwind(struct bun_handle *handle, struct bun_buffer *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
