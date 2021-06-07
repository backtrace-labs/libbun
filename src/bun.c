#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include "bun_structures.h"

bun_handle_t *
bun_create(enum bun_unwind_backend backend)
{
    bun_handle_t *ret = NULL;

    switch (backend) {
        default:
            return NULL;
    }

    if (pthread_mutex_init(&ret->lock, NULL) != 0) {
        ret->destroy(ret);
        return NULL;
    }

    return ret;
}

void
bun_destroy(bun_handle_t *handle)
{

    pthread_mutex_destroy(&handle->lock);

    handle->destroy(handle);
    return;
}

size_t
bun_unwind(bun_handle_t *handle, void *buffer, size_t buffer_size)
{

    return handle->unwind(handle, buffer, buffer_size);
}
