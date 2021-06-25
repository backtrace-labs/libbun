#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bun_unwind_demangle(char *dest, size_t dest_size, const char *src);

#ifdef __cplusplus
}
#endif
