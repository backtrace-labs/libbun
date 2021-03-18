#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <bun/bun.h>

#include "bun_libunwind.h"
#include "bun_internal.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

static size_t libunwind_unwind(void *, void *, size_t);

bun_handle_t initialize_libunwind(struct bun_config *config)
{
    (void *)config;
    bun_handle_t handle = calloc(1, sizeof(struct bun_handle));

    if (handle == NULL)
        return NULL;

    handle->unwind_function = libunwind_unwind;
    return handle;
}

size_t libunwind_unwind(void *ctx, void *dest, size_t buf_size)
{
    unw_cursor_t cursor;
    unw_context_t context;

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    int n = 0;
    while (unw_step(&cursor))
    {
        unw_word_t ip, sp, off;

        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        char symbol[256] = {"<unknown>"};
        char *name = symbol;

        printf("#%-2d 0x%016" PRIxPTR " sp=0x%016" PRIxPTR " %s + 0x%" PRIxPTR "\n",
               ++n,
               (uintptr_t)(ip),
               (uintptr_t)(sp),
               name,
               (uintptr_t)(off));

        if (name != symbol)
            free(name);
    }
    return buf_size;
}
