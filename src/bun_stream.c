#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/syscall.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

#include <bun/stream.h>

static void strcpy_null(char *dest, const char *src);
static size_t strlen_null(const char *str);

#define BUN_HEADER_MAGIC 0xaee9eb7a786a6145

struct bun_writer_reader *
bun_create_writer(void *buffer, size_t size, enum bun_architecture arch)
{
    struct bun_writer_reader *writer = malloc(sizeof(struct bun_writer_reader));
    struct bun_payload_header *hdr;

    if (writer == NULL)
        return NULL;

    writer->buffer = buffer;
    writer->cursor = buffer + sizeof(struct bun_payload_header);
    writer->size = size;

    hdr = buffer;

    hdr->architecture = arch;
    hdr->version = 1;
    hdr->size = sizeof(*hdr);
    hdr->tid = (uint32_t)gettid();

    return writer;
}

struct bun_writer_reader *
bun_create_reader(void *buffer, size_t size)
{
    struct bun_writer_reader *reader;
    struct bun_payload_header *hdr;

    if (size < sizeof(struct bun_payload_header))
        return NULL;

    reader = malloc(sizeof(struct bun_writer_reader));

    if (reader == NULL)
        return NULL;
    
    reader->buffer = buffer;
    reader->cursor = buffer + sizeof(struct bun_payload_header);
    reader->size = size;

    return reader;
}

void
bun_free_writer_reader(struct bun_writer_reader *obj)
{
    free(obj);
}

size_t
bun_frame_write(struct bun_writer_reader *writer, const struct bun_frame *frame)
{
    size_t symbol_length;
    size_t filename_length;
    size_t buffer_available = writer->size - (writer->cursor - writer->buffer);
    size_t would_write;
    size_t register_size = sizeof(uint16_t) + sizeof(uint64_t);
    struct bun_payload_header *header = writer->buffer;

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

    would_write = symbol_length + filename_length + 2 + sizeof(frame->addr) +
        sizeof(frame->line_no) + sizeof(frame->register_count) +
        frame->register_count * register_size;

    /* 2 for null bytes */
    if (would_write > buffer_available)
        return 0;

    memcpy(writer->cursor, &frame->addr, sizeof(frame->addr));
    writer->cursor += sizeof(frame->addr);

    memcpy(writer->cursor, &frame->line_no, sizeof(frame->line_no));
    writer->cursor += sizeof(frame->line_no);

    memcpy(writer->cursor, &frame->offset, sizeof(frame->offset));
    writer->cursor += sizeof(frame->offset);

    strcpy_null(writer->cursor, frame->symbol);
    writer->cursor += symbol_length + 1;

    strcpy_null(writer->cursor, frame->filename);
    writer->cursor += filename_length + 1;

    memcpy(writer->cursor, &frame->register_count,
        sizeof(frame->register_count));
    writer->cursor += sizeof(frame->register_count);

    if (frame->register_count > 0) {
        memcpy(writer->cursor, frame->register_data,
            frame->register_count * register_size);
        writer->cursor += frame->register_count * register_size;
    }

    header->size += would_write;

    return would_write;
}

bool bun_frame_read(struct bun_writer_reader *reader, struct bun_frame *frame)
{
    struct bun_payload_header *header = reader->buffer;
    size_t register_size = sizeof(uint16_t) + sizeof(uint64_t);
    ptrdiff_t buffer_available = reader->size - (reader->cursor - reader->buffer);

    if (buffer_available <= 0)
        return false;

    memcpy(&frame->addr, reader->cursor, sizeof(frame->addr));
    reader->cursor += sizeof(frame->addr);

    memcpy(&frame->line_no, reader->cursor, sizeof(frame->line_no));
    reader->cursor += sizeof(frame->line_no);

    memcpy(&frame->offset, reader->cursor, sizeof(frame->offset));
    reader->cursor += sizeof(frame->offset);

    frame->symbol = reader->cursor;
    reader->cursor += strlen_null(frame->symbol) + 1;

    frame->filename = reader->cursor;
    reader->cursor += strlen_null(frame->filename) + 1;

    memcpy(&frame->register_count, reader->cursor, sizeof(uint16_t));
    reader->cursor += sizeof(frame->register_count);

    if (frame->register_count > 0) {
        frame->register_data = reader->cursor;
        frame->register_buffer_size = frame->register_count * register_size;
        reader->cursor += frame->register_buffer_size;
    }

    return true;
}

uint32_t
bun_header_tid_get(struct bun_writer_reader *reader)
{
    struct bun_payload_header *header = reader->buffer;

    return header->tid;
}

void
bun_header_tid_set(struct bun_writer_reader *writer, uint32_t tid)
{
    struct bun_payload_header *header = writer->buffer;

    header->tid = tid;
}

bool
bun_frame_register_append(struct bun_frame *frame, uint16_t reg,
    uint64_t value)
{
    size_t register_size = sizeof(reg) + sizeof(value);
    size_t max_registers = 0;
    uint8_t *buffer;

    if (frame->register_buffer_size > 0)
        max_registers = frame->register_buffer_size / register_size;

    if (frame->register_count >= max_registers)
        return false;

    buffer = frame->register_data + register_size * frame->register_count;
    memcpy(buffer, &reg, sizeof(reg));
    buffer += sizeof(reg);
    memcpy(buffer, &value, sizeof(value));

    frame->register_count++;
    return true;
}

bool
bun_frame_register_get(struct bun_frame *frame, uint16_t index, uint16_t *reg,
    uint64_t *value)
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

static void
strcpy_null(char *dest, const char *src)
{
    if (src == NULL)
        strcpy(dest, "");
    else
        strcpy(dest, src);
}

static size_t
strlen_null(const char *str)
{
    if (str == NULL)
        return 0;
    else
        return strlen(str);
}
