#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

bool
bun_handle_init(struct bun_handle *handle, enum bun_unwind_backend backend)
{

    switch (backend) {
        default:
            return false;
    }

    return false;
}

void
bun_destroy(struct bun_handle *handle)
{

    handle->destroy(handle);
    return;
}

size_t
bun_unwind(struct bun_handle *handle, void *buffer, size_t buffer_size)
{

    return handle->unwind(handle, buffer, buffer_size);
}
