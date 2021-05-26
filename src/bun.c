#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

#if defined(BUN_LIBUNWIND_ENABLED)
#include "backend/libunwind/bun_libunwind.h"
#endif /* BUN_LIBUNWIND_ENABLED */
#if defined(BUN_LIBBACKTRACE_ENABLED)
#include "backend/libbacktrace/bun_libbacktrace.h"
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
#include "backend/libunwindstack/bun_libunwindstack.h"
#endif /* BUN_LIBUNWINDSTACK_ENABLED */

bool
bun_handle_init(struct bun_handle *handle, enum bun_unwind_backend backend)
{

    switch (backend) {
#if defined(BUN_LIBUNWIND_ENABLED)
        case BUN_BACKEND_LIBUNWIND:
            return bun_internal_initialize_libunwind(handle);
#endif /* BUN_LIBUNWIND_ENABLED */
#if defined(BUN_LIBBACKTRACE_ENABLED)
        case BUN_BACKEND_LIBBACKTRACE:
            return bun_internal_initialize_libbacktrace(handle);
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
        case BUN_BACKEND_LIBUNWINDSTACK:
            return bun_internal_initialize_libunwindstack(handle);
#endif /* BUN_LIBUNWINDSTACK_ENABLED */
        default:
            return false;
    }

    return false;
}

void
bun_handle_deinit(struct bun_handle *handle)
{

    handle->destroy(handle);
    return;
}

size_t
bun_unwind(struct bun_handle *handle, void *buffer, size_t buffer_size)
{

    return handle->unwind(handle, buffer, buffer_size);
}
