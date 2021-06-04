#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include <bun_structures.h>

bun_handle_t *
bun_create(struct bun_config *config)
{
    bun_handle_t *ret = NULL;
    switch (config->unwind_backend) {
        default:
            return NULL;
    }

    if (ret == NULL)
        return NULL;

    if (ret->destroy == NULL) {
        /*
         * if the backend allocated the structure, but did not set the
         * destructor, we're in trouble.
         */
        assert(false);
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
    if (handle == NULL)
        return;

    pthread_mutex_destroy(&handle->lock);

    handle->destroy(handle);
    return;
}

size_t
bun_unwind(bun_handle_t *handle, void *buffer, size_t buffer_size)
{
    size_t bytes_written;

    return handle->unwind(handle, buffer, buffer_size);
}
