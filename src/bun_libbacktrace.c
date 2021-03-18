#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bun_libbacktrace.h"
#include "bun_internal.h"

#include <libbacktrace/backtrace.h>
#include <libbacktrace/backtrace-supported.h>

struct backtrace_context
{
    size_t frames_written;
    size_t frames_left;
    void *data;
};

static size_t libbacktrace_unwind(void *, void *, size_t);

static void error_callback(void *data, const char *msg, int errnum);
static void syminfo_callback(void *data, uintptr_t pc, const char *symname, uintptr_t symval, uintptr_t symsize);
static int full_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function);

bun_handle_t initialize_libbacktrace(struct bun_config *config)
{
    (void *) config;
    bun_handle_t handle = calloc(1, sizeof(struct bun_handle));

    if (handle == NULL)
        return NULL;
    
    handle->unwind_function = libbacktrace_unwind;
    handle->unwind_buffer = config->buffer;
    handle->unwind_buffer_size = config->buffer_size;
    return handle;
}

size_t libbacktrace_unwind(void *ctx, void *dest, size_t buf_size)
{
    (void *)ctx;
    assert(ctx == NULL);

    struct backtrace_state *state = backtrace_create_state(
        NULL /*argv[0]*/,
        BACKTRACE_SUPPORTS_THREADS,
        NULL /*error_callback*/,
        NULL);
    
    struct backtrace_context bt_ctx;

    struct bun_payload_header *hdr = dest;
    hdr->architecture = BUN_ARCH_X86_64;
    hdr->version = 1;

    bt_ctx.data = dest + sizeof(struct bun_payload_header);
    bt_ctx.frames_written = 0;
    bt_ctx.frames_left = (buf_size - sizeof(struct bun_payload_header)) /
        sizeof(struct bun_frame);

    // fprintf(stderr, "%p %lu %lu\n", bt_ctx.data, bt_ctx.frames_left, bt_ctx.frames_written);
    // backtrace_simple(state, 0, simple_callback, error_callback, &bt_ctx);
    backtrace_full(state, 0, full_callback, error_callback, &bt_ctx);
    // fprintf(stderr, "%p %lu %lu\n", bt_ctx.data, bt_ctx.frames_left, bt_ctx.frames_written);
    
    hdr->size = sizeof(struct bun_payload_header) +
        bt_ctx.frames_written * sizeof(struct bun_frame);

    return hdr->size;
}


void error_callback(void *data, const char *msg, int errnum)
{
}

void syminfo_callback (void *data, uintptr_t pc, const char *symname, uintptr_t symval, uintptr_t symsize)
{
    struct backtrace_context *ctx = data;
    struct bun_frame *frame = ctx->data;
    if (symname) {
        strncpy(frame->symbol, symname, sizeof(frame->symbol) - 1);
    } else {
    }
}

int full_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
    struct backtrace_context *ctx = data;
    struct bun_frame *frame = ctx->data;
    if (ctx->frames_left == 0)
        return 0;
    frame->addr = pc;
    ctx->data += sizeof(struct bun_frame);
    ctx->frames_written++;
    ctx->frames_left--;

    if (filename != NULL) {
        strncpy(frame->filename, filename, sizeof(frame->filename) - 1);
    }

    if (function) {
        strncpy(frame->symbol, function, sizeof(frame->symbol) - 1);
    } else {
        backtrace_syminfo (data, pc, syminfo_callback, error_callback, data);
    }
    return 0;
}
