#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include "bun_structures.h"

struct handler_pair
{
    struct sigaction current;
    struct sigaction old;
    void (*signal_handler)(int);
    bool has_old;
};

static struct handler_pair handlers[32];

static bun_handle_t *global_handle;

static void
signal_handler(int signum, siginfo_t *info, void *ucontext)
{
    struct handler_pair *hp = NULL;

    if (signum >= sizeof(handlers)/sizeof(*handlers))
        return;

    hp = &handlers[signum];

    if (hp->signal_handler != NULL) {
        hp->signal_handler(signum);
    }

    if (hp->has_old == true) {
        if (hp->old.sa_flags & SA_SIGINFO) {
            hp->old.sa_sigaction(signum, info, ucontext);
        }
    }

    return;
}

static void
set_signal_handler(int signum, void(*new_handler)(int))
{
    struct handler_pair *hp = NULL;

    if (signum >= sizeof(handlers)/sizeof(*handlers))
        return;

    hp = &handlers[signum];

    hp->current.sa_sigaction = signal_handler;
    sigemptyset(&hp->current.sa_mask);
    hp->current.sa_flags = SA_SIGINFO;

    if (sigaction(signum, NULL, &hp->old) == 0) {
        hp->has_old = true;
    }

    if (sigaction(signum, &hp->current, NULL) == 0) {
        hp->signal_handler = new_handler;
    }

    return;
}

bool
bun_register_signal_handers(bun_handle_t *handle, void(*new_handler)(int))
{
    if (handle == NULL)
        return false;

    pthread_mutex_lock(&handle->lock);

    if (global_handle != NULL && global_handle != handle) {
        pthread_mutex_unlock(&handle->lock);
        return false;
    }

    global_handle = handle;

    set_signal_handler(SIGABRT, new_handler);
    set_signal_handler(SIGBUS, new_handler);
    set_signal_handler(SIGSEGV, new_handler);
    set_signal_handler(SIGILL, new_handler);
    set_signal_handler(SIGSYS, new_handler);
    pthread_mutex_unlock(&handle->lock);
    return true;
}
