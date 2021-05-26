#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include <bun_structures.h>

#include "bun_libunwind.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define REGISTER_GET(cursor, frame, bun_reg, unw_reg, var)                     \
if (unw_get_reg(cursor, unw_reg, &var) == 0)                                   \
    bun_frame_register_append(frame, bun_reg, var);

static size_t libunwind_unwind(void *);
static void destroy_handle(struct bun_handle * handle);

bun_handle_t *_bun_initialize_libunwind(struct bun_config *config)
{
    bun_handle_t *handle = calloc(1, sizeof(struct bun_handle));

    if (handle == NULL)
        return NULL;

    handle->unwind_function = libunwind_unwind;
    handle->unwind_buffer = config->buffer;
    handle->unwind_buffer_size = config->buffer_size;
    handle->arch = config->arch;
    handle->destroy = destroy_handle;
    return handle;
}

size_t libunwind_unwind(void *ctx)
{
    struct bun_handle *handle = ctx;
    struct bun_payload_header *hdr = handle->unwind_buffer;
    struct bun_writer writer;

    bun_writer_init(&writer, handle->unwind_buffer,
        handle->unwind_buffer_size, handle->arch);

    bun_header_backend_set(&writer, BUN_BACKEND_LIBUNWIND);

    unw_cursor_t cursor;
    unw_context_t context;

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    int n = 0;
    while (unw_step(&cursor))
    {
        unw_word_t ip, sp, off, current_register;
        struct bun_frame frame;
        char registers[512] = {0};
        char symbol[256] = {"<unknown>"};
        char *name = symbol;

        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        if (!unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off))
        {
            name = symbol;
        }

        memset(&frame, 0, sizeof(frame));
        frame.symbol = symbol;
        frame.symbol_length = strlen(symbol);
        frame.addr = ip;
        frame.offset = off;
        frame.register_buffer_size = sizeof(registers);
        frame.register_data = registers;

#if defined(__x86_64__)
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RAX, UNW_X86_64_RAX,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RBX, UNW_X86_64_RBX,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RCX, UNW_X86_64_RCX,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RDX, UNW_X86_64_RDX,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RSI, UNW_X86_64_RSI,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RDI, UNW_X86_64_RDI,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RBP, UNW_X86_64_RBP,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R8, UNW_X86_64_R8,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R9, UNW_X86_64_R9,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R10, UNW_X86_64_R10,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R11, UNW_X86_64_R11,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R12, UNW_X86_64_R12,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R13, UNW_X86_64_R13,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R14, UNW_X86_64_R14,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_R15, UNW_X86_64_R15,
            current_register);
        REGISTER_GET(&cursor, &frame, BUN_REGISTER_X86_64_RIP, UNW_X86_64_RIP,
            current_register);
#endif
        bun_frame_write(&writer, &frame);

        if (name != symbol)
            free(name);
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
