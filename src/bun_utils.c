#define _GNU_SOURCE
#include "bun/utils.h"

#include <unistd.h>
#include <sys/syscall.h>

#if defined(BUN_MEMFD_CREATE_AVAILABLE)
#include <sys/memfd.h>
#endif

#if defined(BUN_MEMFD_CREATE_AVAILABLE)
int
bun_memfd_create(const char *name, unsigned int flags)
{

	return memfd_create(name, flags);;
}
#else
int
bun_memfd_create(const char *name, unsigned int flags)
{

	return syscall(SYS_memfd_create, name, flags);
}
#endif
