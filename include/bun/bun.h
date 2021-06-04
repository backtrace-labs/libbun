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
enum bun_unwind_backend {
    BUN_BACKEND_NONE = -1,
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

/*
 * Configuration structure used to initialize the unwinder. The passed buffer
 * will be used to write frames' data up to `buffer_size` bytes. The library
 * *does not* take ownership of the memory passed, so that it is possible to
 * pass a statically allocated buffer
 */
struct bun_config {
    enum bun_unwind_backend unwind_backend;
    enum bun_architecture arch;
};
#define BUN_CONFIG_INITIALIZER { BUN_BACKEND_DEFAULT, BUN_ARCH_DETECTED }

/*
 * Opaque handle for the unwinding object.
 */
struct bun_handle;
typedef struct bun_handle bun_handle_t;

/*
 * Returns a freshly created unwinder for the config, or NULL on failure.
 */
bun_handle_t *bun_create(const struct bun_config *);
/*
 * The de-initialization function.
 */
void bun_destroy(bun_handle_t *);

/*
 * This function unwinds from the current context. The result is stored into the
 * passed buffer.
 *
 * Parameters:
 * - handle - libbun handle
 * - buffer - pointer to the output buffer
 * - buffer_size - maximum number of bytes to write in the output buffer
 *
 * Returns the number of bytes written.
 *
 * This function is safe to use from signal handlers (for backends that allow
 * signal-safe unwinding).
 */
size_t bun_unwind(bun_handle_t *handle, void *buffer, size_t buffer_size);

/*
 * This function registers signal handlers for the following signals:
 * - SIGABRT
 * - SIGBUS
 * - SIGSEGV
 * - SIGILL
 * - SIGSYS
 *
 * The registered signal handler will call bun_unwind() on the buffer passed.
 *
 * If there were registered signal handlers for those signals, they will be
 * called from the libbun's handler, after the
 */
bool bun_sigaction_set(bun_handle_t *handle, void *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif // __cplusplus
