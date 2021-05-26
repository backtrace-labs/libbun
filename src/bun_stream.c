#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <endian.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#include <bun/stream.h>

#include "bun_internal.h"

#define BUN_HEADER_MAGIC 0xaee9eb7a786a6145ull

#if !defined(__ANDROID__)
static pid_t gettid();
#endif

static uint16_t read_le_16(struct bun_reader *src);
static void write_le_16(struct bun_writer *dest, uint16_t value);
static uint64_t read_le_64(struct bun_reader *src);
static void write_le_64(struct bun_writer *dest, uint64_t value);

struct bun_buffer_payload
{
    _Atomic(uint32_t) write_count;
    struct bun_handle *handle;
    struct bun_payload_header header;
};
static_assert(sizeof(struct bun_buffer_payload) == BUN_BUFFER_PAYLOAD_HEADER_SIZE,
    "Expected the payload to be 40 bytes long");

bool
bun_buffer_init(struct bun_buffer *buffer, void *data, size_t size)
{

    if (size < sizeof(struct bun_buffer_payload))
        return false;

    buffer->data = data;
    buffer->size = size;

    memset(buffer->data, 0, sizeof(struct bun_buffer_payload));

    return true;
}

void *
bun_buffer_payload(const struct bun_buffer *buffer)
{
    struct bun_buffer_payload *payload = (void *)buffer->data;

    return &payload->header;
}

size_t
bun_buffer_payload_size(const struct bun_buffer *buffer)
{
    char *payload = bun_buffer_payload(buffer);

    return buffer->size - (payload - buffer->data);
}

bool
bun_writer_init(struct bun_writer *writer, struct bun_buffer *buffer,
    enum bun_architecture arch, struct bun_handle *handle)
{
    struct bun_buffer_payload *buffer_payload = (void *)buffer->data;
    struct bun_payload_header *hdr;

    if (buffer->size < sizeof(struct bun_payload_header))
        return false;

    if (buffer_payload->handle != NULL) {
        if (handle != NULL && handle != buffer_payload->handle)
            return false;
    } else if (handle != NULL) {
        buffer_payload->handle = handle;
    }

    if (buffer_payload->handle != NULL &&
        (buffer_payload->handle->flags & BUN_HANDLE_WRITE_ONCE) != 0) {
        int last = atomic_fetch_add(&buffer_payload->write_count, 1);
        if (last > 0)
            return false;
    }

    hdr = bun_buffer_payload(buffer);
    writer->data.buffer = bun_buffer_payload(buffer);
    writer->data.cursor = bun_buffer_payload(buffer) +
        sizeof(struct bun_payload_header);
    writer->data.size = bun_buffer_payload_size(buffer);
    writer->data.handle = handle;

    hdr->magic = BUN_HEADER_MAGIC;
    hdr->version = 1;
    hdr->architecture = arch;
    hdr->size = sizeof(*hdr);
    hdr->tid = gettid();
    hdr->backend = BUN_BACKEND_NONE;

    return true;
}

bool
bun_reader_init(struct bun_reader *reader, struct bun_buffer *buffer,
    struct bun_handle *handle)
{
    struct bun_payload_header *header = bun_buffer_payload(buffer);

    if (bun_buffer_payload_size(buffer) < sizeof(struct bun_payload_header))
        return false;

    if (header->magic != BUN_HEADER_MAGIC)
        return false;

    reader->data.buffer = bun_buffer_payload(buffer);
    reader->data.cursor = bun_buffer_payload(buffer) + sizeof(struct bun_payload_header);
    reader->data.size = bun_buffer_payload_size(buffer);
    reader->data.handle = handle;

    return true;
}

size_t
bun_frame_write(struct bun_writer *writer, const struct bun_frame *frame)
{
    size_t symbol_length;
    size_t filename_length;
    size_t buffer_available = writer->data.size - (writer->data.cursor -
        writer->data.buffer);
    size_t would_write;
    size_t register_size = sizeof(uint16_t) + sizeof(uint64_t);
    struct bun_payload_header *header = (void *)writer->data.buffer;
    uint64_t temporary_uint64;

    if (frame->symbol_length == 0 && frame->symbol != NULL) {
        symbol_length = strlen(frame->symbol);
    } else {
        symbol_length = frame->symbol_length;
    }

    if (frame->filename_length == 0 && frame->filename != NULL) {
        filename_length = strlen(frame->filename);
    } else {
        filename_length = frame->filename_length;
    }

    /* 2 for null bytes */
    would_write = symbol_length + filename_length + 2 + sizeof(frame->addr) +
        sizeof(frame->line_no) + sizeof(frame->register_count) +
        frame->register_count * register_size;

    if (would_write > buffer_available)
        return 0;

    write_le_64(writer, frame->addr);
    write_le_64(writer, frame->line_no);
    write_le_64(writer, frame->offset);

    strcpy(writer->data.cursor, frame->symbol ? frame->symbol : "");
    writer->data.cursor += symbol_length + 1;

    strcpy(writer->data.cursor, frame->filename ? frame->filename : "");
    writer->data.cursor += filename_length + 1;

    write_le_16(writer, frame->register_count);

    if (frame->register_count > 0) {
        memcpy(writer->data.cursor, frame->register_data,
            frame->register_count * register_size);
        writer->data.cursor += frame->register_count * register_size;
    }

    header->size += would_write;
    return would_write;
}

bool bun_frame_read(struct bun_reader *reader, struct bun_frame *frame)
{
    struct bun_payload_header *header = (void *)reader->data.buffer;
    size_t register_size = sizeof(uint16_t) + sizeof(uint64_t);
    ptrdiff_t buffer_available = reader->data.size - (reader->data.cursor -
        reader->data.buffer);
    (void) header;

    if (buffer_available <= 0)
        return false;

    frame->addr = read_le_64(reader);
    frame->line_no = read_le_64(reader);
    frame->offset = read_le_64(reader);

    frame->symbol = reader->data.cursor;
    reader->data.cursor += strlen(frame->symbol) + 1;

    frame->filename = reader->data.cursor;
    reader->data.cursor += strlen(frame->filename) + 1;

    frame->register_count = read_le_16(reader);

    if (frame->register_count > 0) {
        frame->register_data = (uint8_t *)reader->data.cursor;
        frame->register_buffer_size = frame->register_count * register_size;
        reader->data.cursor += frame->register_buffer_size;
    }

    return true;
}

void
bun_header_tid_set(struct bun_writer *writer, uint32_t tid)
{
    struct bun_payload_header *header = (void *)writer->data.buffer;

    header->tid = tid;
}

uint32_t
bun_header_tid_get(const struct bun_reader *reader)
{
    const struct bun_payload_header *header = (void *)reader->data.buffer;

    return header->tid;
}

void
bun_header_backend_set(struct bun_writer *writer, enum bun_unwind_backend backend)
{
    struct bun_payload_header *header = (void *)writer->data.buffer;

    header->backend = backend;
}

enum bun_unwind_backend
bun_header_backend_get(const struct bun_reader *reader)
{
    const struct bun_payload_header *header = (void *)reader->data.buffer;

    return header->backend;
}

bool
bun_frame_register_append(struct bun_frame *frame, enum bun_register reg,
    uintmax_t value)
{
    size_t register_size = sizeof(reg) + sizeof(value);
    size_t max_registers = 0;
    char *buffer;

    if (frame->register_buffer_size > 0)
        max_registers = frame->register_buffer_size / register_size;

    if (frame->register_count >= max_registers)
        return false;

    buffer = frame->register_data + register_size * frame->register_count;

    reg = htole16(reg);
    memcpy(buffer, &reg, sizeof(uint16_t));
    buffer += sizeof(uint16_t);

    value = htole64(value);
    memcpy(buffer, &value, sizeof(value));

    frame->register_count++;
    return true;
}
static_assert(BUN_REGISTER_COUNT < 65536, "Registers enumerations cannot be "
    "serialized into a two-byte index.");

bool
bun_frame_register_get(struct bun_frame *frame, size_t index,
    enum bun_register *reg, uintmax_t *value)
{
    size_t register_size = sizeof(*reg) + sizeof(*value);
    uint8_t *buffer;

    if (index >= frame->register_count)
        return false;

    buffer = frame->register_data + register_size * index;
    memcpy(reg, buffer, sizeof(*reg));
    buffer += sizeof(*reg);
    memcpy(value, buffer, sizeof(*value));

    return true;
}

#if !defined(__ANDROID__)
static pid_t
gettid()
{
    return (pid_t)syscall(SYS_gettid);
}
#endif

static uint16_t
read_le_16(struct bun_reader *src)
{
    uint16_t value;

    memcpy(&value, src->data.cursor, sizeof(value));
    src->data.cursor += sizeof(value);
    return le16toh(value);
}

static void
write_le_16(struct bun_writer *dest, uint16_t value)
{
    uint16_t le_value = htole16(value);

    memcpy(dest->data.cursor, &le_value, sizeof(le_value));
    dest->data.cursor += sizeof(le_value);
    return;
}

static uint64_t
read_le_64(struct bun_reader *src)
{
    uint64_t value;

    memcpy(&value, src->data.cursor, sizeof(value));
    src->data.cursor += sizeof(value);
    return le64toh(value);
}

static void
write_le_64(struct bun_writer *dest, uint64_t value)
{
    uint64_t le_value = htole64(value);

    memcpy(dest->data.cursor, &le_value, sizeof(le_value));
    dest->data.cursor += sizeof(le_value);
    return;
}
