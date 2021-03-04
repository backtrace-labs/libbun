#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <signal.h>
#include <stdio.h>
#include <dlfcn.h>
static void *(*old_malloc)(size_t);
static int poison = 0;
static void
handler(int unused)
{
        (void)unused;
        poison = !poison;
        return;
}
static void __attribute__((constructor))
poison_init(void)
{
        if (signal(SIGPWR, handler) == SIG_ERR)
                __builtin_trap();
        old_malloc = dlsym(RTLD_NEXT, "malloc");
        if (old_malloc == NULL)
                __builtin_trap();
        return;
}
void *
malloc(size_t r)
{
        if (poison != 0)
                __builtin_trap();
        if (old_malloc == NULL)
                poison_init();
        assert(old_malloc != NULL);
        return old_malloc(r);
}
