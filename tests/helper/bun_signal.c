#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <pthread.h>

#include <bun/bun.h>
#include <bun/stream.h>

/*
 * struct handler_pair keeps the data of a pair of handlers:
 * - current - libbun handler.
 * - old - previous handler set by other code (e.g. Crashpad or Breakpad).
 * - has_old - flag telling whether the other handler is in use.
 */
struct handler_pair {
    struct sigaction current;
    struct sigaction old;
    bool has_old;
};

static struct handler_pair handlers[32];

static struct {
    struct bun_handle *handle;
    void *buffer;
    size_t buffer_size;
    volatile atomic_flag in_use;
    pthread_mutex_t lock;
} signal_data = { .in_use = ATOMIC_FLAG_INIT, .lock = PTHREAD_MUTEX_INITIALIZER };

static void
signal_handler(int signum, siginfo_t *info, void *ucontext)
{
    bool already_in_use;
    struct handler_pair *hp = NULL;

    if (signum >= sizeof(handlers) / sizeof(*handlers))
        return;

    hp = &handlers[signum];

    /* only execute if we're not already in a signal handler */
    already_in_use = atomic_flag_test_and_set(&signal_data.in_use);
    if (already_in_use == false)
        bun_unwind(signal_data.handle, signal_data.buffer,
            signal_data.buffer_size);
    atomic_flag_clear(&signal_data.in_use);

    if (hp->has_old == true) {
        if (hp->old.sa_flags & SA_SIGINFO) {
            hp->old.sa_sigaction(signum, info, ucontext);
        }
    }

    return;
}

static int
set_signal_handler(int signum)
{
    struct handler_pair *hp = NULL;

    if (signum >= sizeof(handlers) / sizeof(*handlers))
        return false;

    hp = &handlers[signum];

    hp->current.sa_sigaction = signal_handler;
    sigemptyset(&hp->current.sa_mask);
    hp->current.sa_flags = SA_SIGINFO;

    /*
     * Only save the old handler once. If the handle is set it means we're
     * registering a new set of signal handlers and we don't want to stack them
     * on top of our previous ones, and want to preserve the originals instead.
     */
    if (signal_data.handle == NULL && sigaction(signum, NULL, &hp->old) == 0) {
        hp->has_old = true;
    }

    if (sigaction(signum, &hp->current, NULL) != 0) {
        return false;
    }

    return true;
}

bool
bun_sigaction_set(struct bun_handle *handle, void *buffer, size_t buffer_size)
{
    const static int signals[] = { SIGABRT, SIGBUS, SIGSEGV, SIGILL, SIGSYS };

    pthread_mutex_lock(&signal_data.lock);

    /*
     * Set handlers for every signal specified in the array. If we fail to set
     * we return false and leave the program in unspecified state to be handled
     * by the client.
     */
    for (size_t i = 0; i < sizeof(signals) / sizeof(*signals); i++) {
        if (set_signal_handler(signals[i]) == false)
            return false;
    }

    signal_data.handle = handle;
    signal_data.buffer = buffer;
    signal_data.buffer_size = buffer_size;

    pthread_mutex_unlock(&signal_data.lock);

    return true;
}
