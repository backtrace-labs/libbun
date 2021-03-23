#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include <bun/bun.h>
#include "bun_internal.h"

struct handler_pair
{
    struct sigaction current;
    struct sigaction old;
    bool has_current;
    bool has_old;
};

struct handler_pair bun_sigsegv;
bun_t *global_handle;

static void signal_handler(int signum)
{
    if (signum == SIGSEGV) {
        // printf("cur: %p, old: %p\n", bun_sigsegv.current, bun_sigsegv.old);
        if (bun_sigsegv.has_current == true)
            bun_sigsegv.current.sa_handler(signum);
        if (bun_sigsegv.has_old == true &&
            sigaction(SIGSEGV, &bun_sigsegv.old, NULL) < 0) {
            raise(signum);
        }
    } else {
    }
}

bool bun_register_signal_handers(bun_t *handle, void(* new_handler)(int))
{
    if (handle == NULL)
        return false;
    
    if (global_handle != NULL && global_handle != handle)
        return false;
    
    global_handle = handle;

    bun_sigsegv.current.sa_handler = new_handler;
    sigemptyset (&bun_sigsegv.current.sa_mask);
    bun_sigsegv.current.sa_flags = 0;

    if (sigaction(SIGSEGV, NULL, &bun_sigsegv.old) < 0) {
        bun_sigsegv.has_old = true;
    }

    if (sigaction(SIGSEGV, &bun_sigsegv.current, NULL) < 0) {
        bun_sigsegv.has_current = true;
    }

    return true;
}
