#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

#if defined(BUN_LIBBACKTRACE_ENABLED)
#include "backend/libbacktrace/bun_libbacktrace.h"
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWIND_ENABLED)
#include "backend/libunwind/bun_libunwind.h"
#endif /* BUN_LIBUNWIND_ENABLED */

bun_t *
bun_create(struct bun_config *config)
{
    switch (config->unwind_backend) {
#if defined(BUN_LIBBACKTRACE_ENABLED)
        case BUN_BACKEND_LIBBACKTRACE:
            return _bun_initialize_libbacktrace(config);
            break;
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWIND_ENABLED)
        case BUN_BACKEND_LIBUNWIND:
            return _bun_initialize_libunwind(config);
            break;
#endif /* BUN_LIBUNWIND_ENABLED */
        default:
            return NULL;
    }
}

void
bun_destroy(bun_t *handle)
{
    if (handle == NULL)
        return;

    if (handle->free_context != NULL)
        handle->free_context(handle->unwinder_context);
    free(handle);
}

enum bun_unwind_result
bun_unwind(bun_t *handle, void **buf, size_t *buf_size)
{
    size_t bytes_written;

    if (handle == NULL || handle->unwind_function == NULL)
        return BUN_UNWIND_FAILURE;

    bytes_written = handle->unwind_function(handle);

    if (buf != NULL && buf_size != NULL) {
        *buf = handle->unwind_buffer;
        *buf_size = bytes_written;
    }
    return BUN_UNWIND_SUCCESS;
}
