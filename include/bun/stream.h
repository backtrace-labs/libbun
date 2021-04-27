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

uint32_t bun_header_tid_get(struct bun_writer_reader *reader);
void bun_header_tid_set(struct bun_writer_reader *writer, uint32_t tid);

#ifdef __cplusplus
}
#endif
