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

#include <stddef.h>

#include <pthread.h>

#include <bun/bun.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t(unwind_function)(struct bun_handle *, void *, size_t);
typedef void(handle_destructor)(struct bun_handle *);

/*
 * bun_handle is the underlying type of the bun_handle_t handle type. It stores
 * the configuration data used to generate reports.
 */
struct bun_handle
{
    unwind_function *unwind;
    handle_destructor *destroy;
    size_t unwind_buffer_size;
    void *unwind_buffer;
    enum bun_architecture arch;
    pthread_mutex_t lock;
};

#ifdef __cplusplus
}
#endif
