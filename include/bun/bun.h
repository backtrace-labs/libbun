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
    BUN_BACKEND_NONE = -1
};

/*
 * Enumeration used to determine the target architecture. Currently only used
 * for informational purposes.
 */
enum bun_architecture {
    BUN_ARCH_X86,
    BUN_ARCH_X86_64,
    BUN_ARCH_ARM,
    BUN_ARCH_ARM64,
    BUN_ARCH_UNKNOWN
};

/*
 * Configuration structure used to initialize the unwinder. The passed buffer
 * will be used to write frames' data up to `buffer_size` bytes. The library
 * *does not* take ownership of the memory passed, so that it is possible to
 * pass a statically allocated buffer
 */
struct bun_config {
    enum bun_unwind_backend unwind_backend;
    size_t buffer_size;
    void *buffer;
    enum bun_architecture arch;
};
#define BUN_CONFIG_INITIALIZER { BUN_BACKEND_NONE, 0, NULL, BUN_ARCH_UNKNOWN }

/*
 * Opaque handle for the unwinding object.
 */
struct bun_handle;
typedef struct bun_handle bun_handle_t;

/*
 * Returns a freshly created unwinder for the config, or NULL on failure.
 */
bun_handle_t *bun_create(struct bun_config *);
/*
 * The de-initialization function.
 */
void bun_destroy(bun_handle_t *);

/*
 * Encodes the result of the unwind function. BUN_UNWIND_PARTIAL is currently
 * unused.
 */
enum bun_unwind_result {
    BUN_UNWIND_FAILURE,
    BUN_UNWIND_SUCCESS
};

/*
 * This function unwinds from the current context. The result is stored into the
 * pre-configured buffer. Additionally, if non-null pointers are passed as the
 * second and third arguments, they're set with the buffer addres and the
 * actual written payload size, which can be less than the buffer size
 */
enum bun_unwind_result bun_unwind(bun_handle_t *handle, void **opt_out_buffer,
    size_t *opt_out_size);

/*
 * This function registers signal handlers for the following signals:
 * - SIGSEGV
 *
 * If there were registered signal handlers for those signals, after the first
 * call, they're reregistered and the signal is reraised to invoke the previous
 * handler.
 */
bool bun_register_signal_handers(bun_handle_t *, void(*)(int));

#ifdef __cplusplus
}
#endif // __cplusplus
