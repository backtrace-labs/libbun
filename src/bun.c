#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>

#include "bun_internal.h"
#if defined(BUN_LIBBACKTRACE_ENABLED)
#include "bun_libbacktrace.h"
#endif //BUN_LIBBACKTRACE_ENABLED
#if defined(BUN_LIBUNWIND_ENABLED)
#include "bun_libunwind.h"
#endif //BUN_LIBUNWIND_ENABLED

bun_handle_t bun_initialize(struct bun_config* config)
{
    switch (config->unwind_backend) {
#if defined(BUN_LIBBACKTRACE_ENABLED)
        case BUN_LIBBACKTRACE:
            return initialize_libbacktrace(config);
            break;
#endif //BUN_LIBBACKTRACE_ENABLED
#if defined(BUN_LIBUNWIND_ENABLED)
        case BUN_LIBUNWIND:
            return initialize_libunwind(config);
            break;
#endif //BUN_LIBUNWIND_ENABLED
#if defined(BUN_LIBUNWINDSTACK_ENABLED)
        case BUN_LIBUNWINDSTACK:
            return NULL;
#endif //BUN_LIBUNWINDSTACK_ENABLED
        default:
            return NULL;
    }
}

void bun_free(bun_handle_t handle)
{
    if (handle != NULL) {
        if (handle->free_context != NULL) {
            handle->free_context(handle->unwinder_context);
        }
        free(handle);
    }
}

bool bun_unwind(bun_handle_t handle, void **buf, size_t *buf_size)
{
    if (handle == NULL || handle->unwind_function == NULL)
        return false;
    
    size_t result;

    result = handle->unwind_function(handle->unwinder_context,
        handle->unwind_buffer, handle->unwind_buffer_size);
    
    if (buf != NULL && buf_size != NULL) {
        *buf = handle->unwind_buffer;
        *buf_size = result;
    }
    return result;
}

