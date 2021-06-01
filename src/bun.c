#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include <bun_structures.h>

bun_handle_t *
bun_create(struct bun_config *config)
{
    switch (config->unwind_backend) {
        default:
            return NULL;
    }
}

void
bun_destroy(bun_handle_t *handle)
{
    if (handle == NULL)
        return;

    if (handle->free_context != NULL)
        handle->free_context(handle->unwinder_context);

    free(handle);

    return;
}

enum bun_unwind_result
bun_unwind(bun_handle_t *handle, void **buf, size_t *buf_size)
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
