/*
 * Copyright (c) 2021 Backtrace I/O, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <bun/bun.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unique stream id for libbun
 */
#define BUN_STREAM_ID 0xbac0000

/*
 * bun_handle is the underlying type of the bun_t handle type. It stores the
 * configuration data used to generate reports.
 */
struct bun_handle
{
    size_t(*unwind_function)(void *);
    void (*free_context)(void *);
    void *unwinder_context;
    size_t unwind_buffer_size;
    void *unwind_buffer;
    enum bun_architecture arch;
};

/*
 * This is the cursor structure used to read/write binary streams generated by
 * libbun. The stream always begins with a header.
 */
struct bun_writer_reader
{
    void *buffer;
    void *cursor;
    size_t size;
};

/*
 * Stream header, used to determine the payload version, its size and its
 * architecture.
 */
struct bun_payload_header
{
    uint64_t magic;
    uint16_t version;
    uint16_t architecture;
    uint32_t size;
    uint32_t tid;
    uint16_t backend;
};

/*
 * Data for a single frame. For strings of characters, ownership is not passed.
 * That is, when writing the user needs to ensure that the pointer is freed, and
 * when reading, the user needs not to free the received pointers.
 */
struct bun_frame
{
    uint64_t addr;
    char *symbol;
    size_t symbol_length;
    char *filename;
    size_t filename_length;
    size_t line_no;
    size_t offset;
    uint16_t register_count;
    uint16_t register_buffer_size;
    uint8_t *register_data;
};

enum bun_register {
/* x86-64 specific registers */
    BUN_REGISTER_X86_64_RAX,
    BUN_REGISTER_X86_64_RBX,
    BUN_REGISTER_X86_64_RCX,
    BUN_REGISTER_X86_64_RDX,
    BUN_REGISTER_X86_64_RSI,
    BUN_REGISTER_X86_64_RDI,
    BUN_REGISTER_X86_64_RBP,
    BUN_REGISTER_X86_64_RSP,
    BUN_REGISTER_X86_64_R8,
    BUN_REGISTER_X86_64_R9,
    BUN_REGISTER_X86_64_R10,
    BUN_REGISTER_X86_64_R11,
    BUN_REGISTER_X86_64_R12,
    BUN_REGISTER_X86_64_R13,
    BUN_REGISTER_X86_64_R14,
    BUN_REGISTER_X86_64_R15,
    BUN_REGISTER_X86_64_RIP,
/* x86 specific registers */
    BUN_REGISTER_X86_EAX,
    BUN_REGISTER_X86_EBX,
    BUN_REGISTER_X86_ECX,
    BUN_REGISTER_X86_EDX,
    BUN_REGISTER_X86_ESI,
    BUN_REGISTER_X86_EDI,
    BUN_REGISTER_X86_EBP,
    BUN_REGISTER_X86_ESP,
    BUN_REGISTER_X86_EIP,
/* aarch64 specific registers */
    BUN_REGISTER_AARCH64_X0,
    BUN_REGISTER_AARCH64_X1,
    BUN_REGISTER_AARCH64_X2,
    BUN_REGISTER_AARCH64_X3,
    BUN_REGISTER_AARCH64_X4,
    BUN_REGISTER_AARCH64_X5,
    BUN_REGISTER_AARCH64_X6,
    BUN_REGISTER_AARCH64_X7,
    BUN_REGISTER_AARCH64_X8,
    BUN_REGISTER_AARCH64_X9,
    BUN_REGISTER_AARCH64_X10,
    BUN_REGISTER_AARCH64_X11,
    BUN_REGISTER_AARCH64_X12,
    BUN_REGISTER_AARCH64_X13,
    BUN_REGISTER_AARCH64_X14,
    BUN_REGISTER_AARCH64_X15,
    BUN_REGISTER_AARCH64_X16,
    BUN_REGISTER_AARCH64_X17,
    BUN_REGISTER_AARCH64_X18,
    BUN_REGISTER_AARCH64_X19,
    BUN_REGISTER_AARCH64_X20,
    BUN_REGISTER_AARCH64_X21,
    BUN_REGISTER_AARCH64_X22,
    BUN_REGISTER_AARCH64_X23,
    BUN_REGISTER_AARCH64_X24,
    BUN_REGISTER_AARCH64_X25,
    BUN_REGISTER_AARCH64_X26,
    BUN_REGISTER_AARCH64_X27,
    BUN_REGISTER_AARCH64_X28,
    BUN_REGISTER_AARCH64_X29,
    BUN_REGISTER_AARCH64_X30,
    BUN_REGISTER_AARCH64_X31,
    BUN_REGISTER_AARCH64_PC,
    BUN_REGISTER_AARCH64_PSTATE,
/* arm specific registers*/
    BUN_REGISTER_ARM_R0,
    BUN_REGISTER_ARM_R1,
    BUN_REGISTER_ARM_R2,
    BUN_REGISTER_ARM_R3,
    BUN_REGISTER_ARM_R4,
    BUN_REGISTER_ARM_R5,
    BUN_REGISTER_ARM_R6,
    BUN_REGISTER_ARM_R7,
    BUN_REGISTER_ARM_R8,
    BUN_REGISTER_ARM_R9,
    BUN_REGISTER_ARM_R10,
    BUN_REGISTER_ARM_R11,
    BUN_REGISTER_ARM_R12,
    BUN_REGISTER_ARM_R13,
    BUN_REGISTER_ARM_R14,
    BUN_REGISTER_ARM_R15,
    BUN_REGISTER_ARM_PSTATE,
/* count */
    BUN_REGISTER_COUNT
};

/*
 * Create and initialize a writer for the specified buffer and architecture.
 */
struct bun_writer_reader *bun_create_writer(void *buffer, size_t size,
    enum bun_architecture arch);

/*
 * Create and initialize a reader for the specified buffer.
 */
struct bun_writer_reader *bun_create_reader(void *buffer, size_t size);

/*
 * De-initialize the writer or reader.
 */
void bun_free_writer_reader(struct bun_writer_reader *);

/*
 * Serialize a single frame at the current position of the writer's cursor. This
 * library strives to waste as little space as pssible when doing this.
 */
size_t bun_frame_write(struct bun_writer_reader *writer,
    const struct bun_frame *frame);

/*
 * Deserialize a single frame at the current position of the reader's cursor.
 * The function will return true if the structure under the pointer has been
 * updated with next frame's data.
 * If the deserialization cannot be performed (e.g. the cursor reached the end
 * of the data), this function will return false.
 */
bool bun_frame_read(struct bun_writer_reader *writer, struct bun_frame *frame);

/*
 * Set thread id of the reporting thread.
 */
void bun_header_tid_set(struct bun_writer_reader *writer, uint32_t tid);

/*
 * Get thread id of the reporting thread.
 */
uint32_t bun_header_tid_get(const struct bun_writer_reader *reader);

/*
 * Set thread id of the reporting thread.
 */
void bun_header_backend_set(struct bun_writer_reader *writer, uint16_t backend);

/*
 * Get thread id of the reporting thread.
 */
uint16_t bun_header_backend_get(const struct bun_writer_reader *reader);

/*
 * Append the register value to the specified frame.
 *
 * Returns false if there is no space left in the frame register buffer.
 * Returns true otherwise.
 */
bool bun_frame_register_append(struct bun_frame *, uint16_t reg,
    uint64_t value);

/*
 * Gets the register value and type at the index.
 *
 * index must be less than frame->register_count
 *
 * Returns true if the values have been set.
 */
bool bun_frame_register_get(struct bun_frame *, uint16_t index, uint16_t *reg,
    uint64_t *value);

#ifdef __cplusplus
}
#endif
