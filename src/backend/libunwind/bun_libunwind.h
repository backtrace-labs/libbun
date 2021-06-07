#pragma once

#include <bun/bun.h>

/*
 * Initialize the libunwind handler backend. This function is only meant
 * for internal use.
 */
bun_handle_t *bun_internal_initialize_libunwind(void);
