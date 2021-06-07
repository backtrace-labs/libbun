#pragma once

#include <bun/bun.h>

/*
 * Initialize the libunwind handler backend. This function is only meant
 * for internal use.
 */
bool bun_internal_initialize_libunwind(struct bun_handle *handle);
