/* for libunwind */
#define UNW_LOCAL_ONLY

#include "bun_libunwind.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include <bun_structures.h>

#include <libunwind.h>

#define REGISTER_GET(cursor, frame, bun_reg, unw_reg, var)                    \
do { if (unw_get_reg(cursor, unw_reg, &var) == 0)                             \
    bun_frame_register_append(frame, bun_reg, var); } while (false)

/*
 * Performs the actual unwinding using libunwind
 */
static size_t libunwind_unwind(bun_handle_t *, void *, size_t);

/*
 * Handle destructor, calls bun_handle_deinit_internal() internally, to provide
 * inheritance-like functionality for bun_handle.
 */
static void destroy_handle(struct bun_handle * handle);

bun_handle_t *
bun_internal_initialize_libunwind(void)
{
    bun_handle_t *handle = calloc(1, sizeof(struct bun_handle));

    if (handle == NULL)
        return NULL;

    handle->unwind = libunwind_unwind;
    handle->destroy = destroy_handle;
    handle->arch = BUN_ARCH_DETECTED;
    return handle;
}

size_t
libunwind_unwind(bun_handle_t *handle, void *buffer, size_t buffer_size)
{
    struct bun_payload_header *hdr = buffer;
    struct bun_writer writer;
    unw_cursor_t cursor;
    unw_context_t context;
    int n = 0;

    bun_writer_init(&writer, buffer, buffer_size, handle->arch);

    bun_header_backend_set(&writer, BUN_BACKEND_LIBUNWIND);

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0) {
        unw_word_t ip, sp, off, current_register;
        struct bun_frame frame;
        char registers[512] = {0};
        char symbol[256] = {"<unknown>"};
        int get_proc_name_result;

        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        get_proc_name_result = unw_get_proc_name(&cursor, symbol,
            sizeof(symbol), &off);

        /*
         * Don't overwrite the symbol for UNW_ENOMEM because it returns
         * a partially useful name
         */
        if (get_proc_name_result != 0 && get_proc_name_result != UNW_ENOMEM) {
            strcpy(symbol, "<unknown>");
        }

        memset(&frame, 0, sizeof(frame));
        frame.symbol = symbol;
        frame.symbol_length = strlen(symbol);
        frame.addr = ip;
        frame.offset = off;
        frame.register_buffer_size = sizeof(registers);
        frame.register_data = registers;

#define X(NAME) REGISTER_GET(&cursor, &frame, BUN_REGISTER_ ## NAME,          \
    UNW_ ## NAME, current_register);
#if defined(__x86_64__)
        X(X86_64_RAX);
        X(X86_64_RBX);
        X(X86_64_RCX);
        X(X86_64_RDX);
        X(X86_64_RSI);
        X(X86_64_RDI);
        X(X86_64_RBP);
        X(X86_64_R8);
        X(X86_64_R9);
        X(X86_64_R10);
        X(X86_64_R11);
        X(X86_64_R12);
        X(X86_64_R13);
        X(X86_64_R14);
        X(X86_64_R15);
        X(X86_64_RIP);
#endif
#undef X
        if (bun_frame_write(&writer, &frame) == 0)
            return 0;
    }
    return hdr->size;
}

static void
destroy_handle(struct bun_handle *handle)
{
    bun_handle_deinit_internal(handle);
    free(handle);
    return;
}
