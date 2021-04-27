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
    hdr->size = 0;
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
        sizeof(frame->line_no);

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

    header->size += would_write;

    return would_write;
}

bool bun_frame_read(struct bun_writer_reader *reader, struct bun_frame *frame)
{
    struct bun_payload_header *header = reader->buffer;
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

static void strcpy_null(char *dest, const char *src)
{
    if (src == NULL)
        strcpy(dest, "");
    else
        strcpy(dest, src);
}

static size_t strlen_null(const char *str)
{
    if (str == NULL)
        return 0;
    else
        return strlen(str);
}

