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
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
#include "backend/libunwindstack/bun_libunwindstack.h"
#endif /* BUN_LIBUNWINDSTACK_ENABLED */

bun_t *bun_create(struct bun_config* config)
{
    switch (config->unwind_backend) {
#if defined(BUN_LIBBACKTRACE_ENABLED)
        case BUN_LIBBACKTRACE:
            return _bun_initialize_libbacktrace(config);
            break;
#endif /* BUN_LIBBACKTRACE_ENABLED */
#if defined(BUN_LIBUNWIND_ENABLED)
        case BUN_LIBUNWIND:
            return _bun_initialize_libunwind(config);
            break;
#endif /* BUN_LIBUNWIND_ENABLED */
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
        case BUN_LIBUNWINDSTACK:
            return _bun_initialize_libunwindstack(config);
#endif /* BUN_LIBUNWINDSTACK_ENABLED */
        default:
            return NULL;
    }
}

void bun_destroy(bun_t *handle)
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
    enum bun_unwind_result result;
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

