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

#include <bun/bun.h>
#include <bun/stream.h>

template<typename Unwind, typename Destroy>
static bool
initialize_test_backend(struct bun_handle *handle, Unwind &&u, Destroy &&d)
{
    using decayed_unwind = std::decay_t<Unwind>;
    using decayed_destroy = std::decay_t<Destroy>;
    using vtable = std::tuple<decayed_unwind, decayed_destroy>;

    *handle = {};

    handle->backend_context = static_cast<void *>(new vtable{
        std::forward<Unwind>(u), std::forward<Destroy>(d)});

    handle->unwind = +[](
        struct bun_handle *handle, struct bun_buffer *buffer) -> size_t {
        auto *ctx = static_cast<const vtable *>(handle->backend_context);
        return std::get<decayed_unwind>(*ctx)(handle, buffer);
    };

    handle->destroy = +[](struct bun_handle *handle) {
        auto *ctx = static_cast<vtable *>(handle->backend_context);
        std::get<decayed_destroy>(*ctx)(handle);
        delete ctx;
    };

    return true;
}
