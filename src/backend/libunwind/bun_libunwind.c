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

#include <libunwind.h>

#define REGISTER_GET(cursor, frame, bun_reg, unw_reg, var)                    \
do { if (unw_get_reg(cursor, unw_reg, &var) == 0)                             \
    bun_frame_register_append(frame, bun_reg, var); } while (false)

/*
 * Performs the actual unwinding using libunwind.
 */
static size_t libunwind_unwind(struct bun_handle *handle, void *buffer,
    size_t buffer_size);

/*
 * Handle destructor, calls bun_handle_deinit_internal() internally, to provide
 * inheritance-like functionality for bun_handle.
 */
static void destroy_handle(struct bun_handle * handle);

bool
bun_internal_initialize_libunwind(struct bun_handle *handle)
{

    handle->unwind = libunwind_unwind;
    handle->destroy = destroy_handle;
    return handle;
}

size_t
libunwind_unwind(struct bun_handle *handle, void *buffer, size_t buffer_size)
{
    struct bun_payload_header *hdr = buffer;
    struct bun_writer writer;
    unw_cursor_t cursor;
    unw_context_t context;
    int n = 0;

    bun_writer_init(&writer, buffer, buffer_size, BUN_ARCH_DETECTED);

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

// #define X(NAME) REGISTER_GET(&cursor, &frame, BUN_REGISTER_ ## NAME,          \
//     UNW_ ## NAME, current_register);
#if defined(__x86_64__)
        static const struct {
            enum bun_register bun_reg;
            x86_64_regnum_t unw_reg;
        } register_map[] = {
            {BUN_REGISTER_X86_64_RAX, UNW_X86_64_RAX},
            {BUN_REGISTER_X86_64_RBX, UNW_X86_64_RBX},
            {BUN_REGISTER_X86_64_RCX, UNW_X86_64_RCX},
            {BUN_REGISTER_X86_64_RDX, UNW_X86_64_RDX},
            {BUN_REGISTER_X86_64_RSI, UNW_X86_64_RSI},
            {BUN_REGISTER_X86_64_RDI, UNW_X86_64_RDI},
            {BUN_REGISTER_X86_64_RBP, UNW_X86_64_RBP},
            {BUN_REGISTER_X86_64_R8, UNW_X86_64_R8},
            {BUN_REGISTER_X86_64_R9, UNW_X86_64_R9},
            {BUN_REGISTER_X86_64_R10, UNW_X86_64_R10},
            {BUN_REGISTER_X86_64_R11, UNW_X86_64_R11},
            {BUN_REGISTER_X86_64_R12, UNW_X86_64_R12},
            {BUN_REGISTER_X86_64_R13, UNW_X86_64_R13},
            {BUN_REGISTER_X86_64_R14, UNW_X86_64_R14},
            {BUN_REGISTER_X86_64_R15, UNW_X86_64_R15},
            {BUN_REGISTER_X86_64_RIP, UNW_X86_64_RIP}
        };
#elif defined(__i386__)
        static const struct {
            enum bun_register bun_reg;
            x86_regnum_t unw_reg;
        } register_map[] = {
            {BUN_REGISTER_X86_EAX, UNW_X86_EAX},
            {BUN_REGISTER_X86_EBX, UNW_X86_EBX},
            {BUN_REGISTER_X86_ECX, UNW_X86_ECX},
            {BUN_REGISTER_X86_EDX, UNW_X86_EDX},
            {BUN_REGISTER_X86_ESI, UNW_X86_ESI},
            {BUN_REGISTER_X86_EDI, UNW_X86_EDI},
            {BUN_REGISTER_X86_EBP, UNW_X86_EBP},
            {BUN_REGISTER_X86_ESP, UNW_X86_ESP},
            {BUN_REGISTER_X86_EIP, UNW_X86_EIP},
            {BUN_REGISTER_X86_END, UNW_X86_END},
        };
#elif defined(__aarch64__)
        static const struct {
            enum bun_register bun_reg;
            aarch64_regnum_t unw_reg;
        } register_map[] = {
            {BUN_REGISTER_AARCH64_X0, UNW_AARCH64_X0},
            {BUN_REGISTER_AARCH64_X1, UNW_AARCH64_X1},
            {BUN_REGISTER_AARCH64_X2, UNW_AARCH64_X2},
            {BUN_REGISTER_AARCH64_X3, UNW_AARCH64_X3},
            {BUN_REGISTER_AARCH64_X4, UNW_AARCH64_X4},
            {BUN_REGISTER_AARCH64_X5, UNW_AARCH64_X5},
            {BUN_REGISTER_AARCH64_X6, UNW_AARCH64_X6},
            {BUN_REGISTER_AARCH64_X7, UNW_AARCH64_X7},
            {BUN_REGISTER_AARCH64_X8, UNW_AARCH64_X8},
            {BUN_REGISTER_AARCH64_X9, UNW_AARCH64_X9},
            {BUN_REGISTER_AARCH64_X10, UNW_AARCH64_X10},
            {BUN_REGISTER_AARCH64_X11, UNW_AARCH64_X11},
            {BUN_REGISTER_AARCH64_X12, UNW_AARCH64_X12},
            {BUN_REGISTER_AARCH64_X13, UNW_AARCH64_X13},
            {BUN_REGISTER_AARCH64_X14, UNW_AARCH64_X14},
            {BUN_REGISTER_AARCH64_X15, UNW_AARCH64_X15},
            {BUN_REGISTER_AARCH64_X16, UNW_AARCH64_X16},
            {BUN_REGISTER_AARCH64_X17, UNW_AARCH64_X17},
            {BUN_REGISTER_AARCH64_X18, UNW_AARCH64_X18},
            {BUN_REGISTER_AARCH64_X19, UNW_AARCH64_X19},
            {BUN_REGISTER_AARCH64_X20, UNW_AARCH64_X20},
            {BUN_REGISTER_AARCH64_X21, UNW_AARCH64_X21},
            {BUN_REGISTER_AARCH64_X22, UNW_AARCH64_X22},
            {BUN_REGISTER_AARCH64_X23, UNW_AARCH64_X23},
            {BUN_REGISTER_AARCH64_X24, UNW_AARCH64_X24},
            {BUN_REGISTER_AARCH64_X25, UNW_AARCH64_X25},
            {BUN_REGISTER_AARCH64_X26, UNW_AARCH64_X26},
            {BUN_REGISTER_AARCH64_X27, UNW_AARCH64_X27},
            {BUN_REGISTER_AARCH64_X28, UNW_AARCH64_X28},
            {BUN_REGISTER_AARCH64_X29, UNW_AARCH64_X29},
            {BUN_REGISTER_AARCH64_X30, UNW_AARCH64_X30},
            {BUN_REGISTER_AARCH64_X31, UNW_AARCH64_X31},
            {BUN_REGISTER_AARCH64_PC, UNW_AARCH64_PC},
            {BUN_REGISTER_AARCH64_PSTATE, UNW_AARCH64_PSTATE}
        };
#elif defined(__arm__)
        static const struct {
            enum bun_register bun_reg;
            arm_regnum_t unw_reg;
        } register_map[] = {
            {BUN_REGISTER_ARM_R0, UNW_ARM_R0},
            {BUN_REGISTER_ARM_R1, UNW_ARM_R1},
            {BUN_REGISTER_ARM_R2, UNW_ARM_R2},
            {BUN_REGISTER_ARM_R3, UNW_ARM_R3},
            {BUN_REGISTER_ARM_R4, UNW_ARM_R4},
            {BUN_REGISTER_ARM_R5, UNW_ARM_R5},
            {BUN_REGISTER_ARM_R6, UNW_ARM_R6},
            {BUN_REGISTER_ARM_R7, UNW_ARM_R7},
            {BUN_REGISTER_ARM_R8, UNW_ARM_R8},
            {BUN_REGISTER_ARM_R9, UNW_ARM_R9},
            {BUN_REGISTER_ARM_R10, UNW_ARM_R10},
            {BUN_REGISTER_ARM_R11, UNW_ARM_R11},
            {BUN_REGISTER_ARM_R12, UNW_ARM_R12},
            {BUN_REGISTER_ARM_R13, UNW_ARM_R13},
            {BUN_REGISTER_ARM_R14, UNW_ARM_R14},
            {BUN_REGISTER_ARM_R15, UNW_ARM_R15},
            {BUN_REGISTER_ARM_PSTATE, UNW_ARM_PSTATE}
        };
#endif

        for (size_t i = 0; i < sizeof(register_map)/sizeof(*register_map); i++) {
            REGISTER_GET(&cursor, &frame, register_map[i].bun_reg,
                register_map[i].unw_reg, current_register);
        }
        if (bun_frame_write(&writer, &frame) == 0)
            return 0;
    }
    return hdr->size;
}

static void
destroy_handle(struct bun_handle *handle)
{

    return;
}
