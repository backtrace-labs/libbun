#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bun/stream.h>

#include "bun_libbacktrace.h"

#include <libbacktrace/backtrace.h>
#include <libbacktrace/backtrace-supported.h>

#define LIBBACKTRACE_MAX_SYMBOL_NAME 256

struct backtrace_context
{
    struct backtrace_state *state;
    struct bun_writer_reader *writer;
};

static size_t libbacktrace_unwind(void *);
static void error_callback(void *data, const char *msg, int errnum);
static void syminfo_callback(void *data, uintptr_t pc, const char *symname,
    uintptr_t symval, uintptr_t symsize);
static int full_callback(void *data, uintptr_t pc, const char *filename,
    int lineno, const char *function);

bun_t *_bun_initialize_libbacktrace(struct bun_config *config)
{
    (void *) config;
    bun_t *handle = calloc(1, sizeof(struct bun_handle));

    if (handle == NULL)
        return NULL;

    handle->unwind_function = libbacktrace_unwind;
    handle->unwind_buffer = config->buffer;
    handle->unwind_buffer_size = config->buffer_size;
    handle->arch = config->arch;
    return handle;
}

size_t libbacktrace_unwind(void *ctx)
{
    struct bun_handle *handle = ctx;
    struct bun_payload_header *hdr = handle->unwind_buffer;

    assert(ctx != NULL);

    struct backtrace_state *state = backtrace_create_state(
        NULL /*argv[0]*/,
        BACKTRACE_SUPPORTS_THREADS,
        NULL /*error_callback*/,
        NULL);

    struct backtrace_context bt_ctx;

    bt_ctx.state = state;
    bt_ctx.writer = bun_create_writer(handle->unwind_buffer,
        handle->unwind_buffer_size, handle->arch);

    bun_header_backend_set(bt_ctx.writer, BUN_BACKEND_LIBBACKTRACE);

    backtrace_full(state, 0, full_callback, error_callback, &bt_ctx);
    bun_free_writer_reader(bt_ctx.writer);
    return hdr->size;
}

void error_callback(void *data, const char *msg, int errnum)
{
}

void syminfo_callback(void *data, uintptr_t pc, const char *symname,
    uintptr_t symval, uintptr_t symsize)
{
    char *str = data;

    if (symname != NULL) {
        strncpy(str, symname, LIBBACKTRACE_MAX_SYMBOL_NAME - 1);
        str[LIBBACKTRACE_MAX_SYMBOL_NAME - 1] = '\0';
    }
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
        bun_frame_write(ctx->writer, &frame);
    } else {
        char function_name[LIBBACKTRACE_MAX_SYMBOL_NAME] = { '\0' };
        backtrace_syminfo(ctx->state, pc, syminfo_callback, error_callback,
            function_name);
        frame.symbol = function_name;
        frame.symbol_length = strlen(frame.symbol);
        bun_frame_write(ctx->writer, &frame);
    }
    return 0;
}
