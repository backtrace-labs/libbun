#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include <bun/bun.h>
#include <bun/stream.h>

struct handler_pair
{
    struct sigaction current;
    struct sigaction old;
    void (*overridden_handler)(int);
    bool overridden;
    bool has_current;
    bool has_old;
};

static struct handler_pair handlers[32];

static bun_t *global_handle;

static void
signal_handler(int signum)
{
    struct handler_pair *hp = NULL;

    if (signum >= sizeof(handlers)/sizeof(*handlers))
        return;

    hp = &handlers[signum];

    if (hp->overridden == false)
        return;

    if (hp->has_current == true)
        hp->overridden_handler(signum);

    if (hp->has_old == true &&
        sigaction(signum, &hp->old, NULL) < 0) {
        raise(signum);
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

    hp->current.sa_handler = signal_handler;
    sigemptyset(&hp->current.sa_mask);
    hp->current.sa_flags = 0;
    hp->overridden = true;
    hp->overridden_handler = new_handler;

    if (sigaction(signum, NULL, &hp->old) == 0) {
        hp->has_old = true;
    }

    if (sigaction(signum, &hp->current, NULL) == 0) {
        hp->has_current = true;
    }

    return;
}

bool
bun_register_signal_handers(bun_t *handle, void(*new_handler)(int))
{
    if (handle == NULL)
        return false;

    if (global_handle != NULL && global_handle != handle)
        return false;

    global_handle = handle;

    set_signal_handler(SIGABRT, new_handler);
    set_signal_handler(SIGBUS, new_handler);
    set_signal_handler(SIGSEGV, new_handler);
    set_signal_handler(SIGILL, new_handler);
    set_signal_handler(SIGSYS, new_handler);
    return true;
}
