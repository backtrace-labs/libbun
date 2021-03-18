#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <bun/bun.h>
#include "bun_internal.h"

struct handler_pair
{
    __sighandler_t current;
    __sighandler_t old;
};

struct handler_pair bun_sigsegv;
bun_handle_t global_handle;

static void signal_handler(int signum)
{
    if (signum == SIGSEGV) {
        // printf("cur: %p, old: %p\n", bun_sigsegv.current, bun_sigsegv.old);
        if (bun_sigsegv.current != NULL)
            bun_sigsegv.current(signum);
        if (bun_sigsegv.old != NULL) {
            signal(signum, bun_sigsegv.old);
            raise(signum);
        }
    } else {
    }
}

bool bun_register_signal_handers(bun_handle_t handle, void(* new_handler)(int))
{
    if (handle == NULL)
        return false;
    
    if (global_handle != NULL)
        return false;
    
    global_handle = handle;

    bun_sigsegv.current = new_handler;
    bun_sigsegv.old = signal(SIGSEGV, signal_handler);

    return true;
}
