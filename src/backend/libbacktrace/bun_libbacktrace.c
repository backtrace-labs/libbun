#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bun/stream.h>

#include "bun_libbacktrace.h"

#include <libbacktrace/backtrace.h>
#include <libbacktrace/backtrace-supported.h>

#define LIBBACKTRACE_MAX_SYMBOL_NAME 256

#define BUN_WRITE_ERROR -1

struct backtrace_context
{
    bun_writer_t writer;
};

static size_t libbacktrace_unwind(struct bun_handle *, void *, size_t);
static void error_callback(void *data, const char *msg, int errnum);
static void syminfo_callback(void *data, uintptr_t pc, const char *symname,
    uintptr_t symval, uintptr_t symsize);
static int full_callback(void *data, uintptr_t pc, const char *filename,
    int lineno, const char *function);

static struct backtrace_state *get_backtrace_state()
{
    static struct backtrace_state *state;

    if (state == NULL)
        state = backtrace_create_state(
            NULL /*argv[0]*/,
            BACKTRACE_SUPPORTS_THREADS,
            NULL /*error_callback*/,
            NULL);
    return state;
}

static void libbacktrace_destroy()
{
    return;
}

bool bun_internal_initialize_libbacktrace(struct bun_handle *handle)
{
    /* Ensure we precompute the state for further operations. */
    (void) get_backtrace_state();
    handle->unwind = libbacktrace_unwind;
    handle->destroy = libbacktrace_destroy;
    return true;
}

size_t libbacktrace_unwind(struct bun_handle *handle, void *buffer,
    size_t buffer_size)
{
    struct backtrace_context bt_ctx;
    struct bun_payload_header *hdr = buffer;

    bun_writer_init(&bt_ctx.writer, buffer, buffer_size, BUN_ARCH_DETECTED);

    bun_header_backend_set(&bt_ctx.writer, BUN_BACKEND_LIBBACKTRACE);

    if (backtrace_full(get_backtrace_state(), 0, full_callback, error_callback,
        &bt_ctx) != 0)
        return 0;

    return hdr->size;
}

void error_callback(void *data, const char *msg, int errnum)
{
    return;
}

void syminfo_callback(void *data, uintptr_t pc, const char *symname,
    uintptr_t symval, uintptr_t symsize)
{
    char *str = data;

    if (symname != NULL) {
        strncpy(str, symname, LIBBACKTRACE_MAX_SYMBOL_NAME - 1);
        str[LIBBACKTRACE_MAX_SYMBOL_NAME - 1] = '\0';
    }
    return;
}

int full_callback(void *data, uintptr_t pc, const char *filename, int lineno,
    const char *function)
{
    struct backtrace_context *ctx = data;

    struct bun_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.addr = (uint64_t)pc;
    frame.line_no = lineno;
    frame.symbol = (char *)function;
    frame.filename = (char *)filename;

    if (function == NULL) {
        if (bun_frame_write(&ctx->writer, &frame) == 0)
            return BUN_WRITE_ERROR;
    } else {
        char function_name[LIBBACKTRACE_MAX_SYMBOL_NAME] = { '\0' };
        backtrace_syminfo(get_backtrace_state(), pc, syminfo_callback,
            error_callback, function_name);
        frame.symbol = function_name;
        frame.symbol_length = strlen(frame.symbol);
        if (bun_frame_write(&ctx->writer, &frame) == 0)
            return BUN_WRITE_ERROR;
    }
    return 0;
}
