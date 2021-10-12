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

#include "bun/stream.h"
#include "bun/utils.h"

#include "bun_internal.h"
#include "register_to_string.h"

#define BUN_HEADER_MAGIC 0xaee9eb7a786a6145ull
#define REGISTER_SIZE (sizeof(uint16_t) + sizeof(uint64_t))

static bool is_safe_access(const struct bun_writer_reader_base *, size_t bytes);
static uint16_t read_le_16(struct bun_reader *src);
static void write_le_16(struct bun_writer *dest, uint16_t value);
static uint64_t read_le_64(struct bun_reader *src);
static void write_le_64(struct bun_writer *dest, uint64_t value);
static ssize_t safe_strlen(struct bun_reader *src);

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

#if UINTPTR_MAX == 0xFFFFFFFFu
#define BUN_32BIT
#define PADDING uint32_t UNIQUE_NAME(foo);
#else
#define BUN_64BIT
#define PADDING
#endif

struct bun_buffer_payload {
	_Atomic(uint32_t) write_count;
	PADDING;
	struct bun_handle *handle;
	PADDING;
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
	writer->data.overflow = false;

	hdr->magic = BUN_HEADER_MAGIC;
	hdr->version = 1;
	hdr->architecture = arch;
	hdr->size = sizeof(*hdr);
	hdr->tid = bun_gettid();
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
	reader->data.cursor = bun_buffer_payload(buffer) +
	    sizeof(struct bun_payload_header);
	reader->data.size = bun_buffer_payload_size(buffer);
	reader->data.handle = handle;
	reader->data.overflow = false;

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
	struct bun_payload_header *header = (void *)writer->data.buffer;

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
	    frame->register_count * REGISTER_SIZE;

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
		    frame->register_count * REGISTER_SIZE);
		writer->data.cursor += frame->register_count * REGISTER_SIZE;
	}

	header->size += would_write;
	return would_write;
}

bool
bun_frame_read(struct bun_reader *reader, struct bun_frame *frame)
{
	struct bun_payload_header *header = (void *)reader->data.buffer;
	char *const initial_cursor_value = reader->data.cursor;
	const size_t offset = reader->data.cursor - reader->data.buffer;
	ptrdiff_t buffer_available = header->size - offset;
	ssize_t length = -1;
	(void) header;

	if (reader->data.size - offset <= 0)
		return false;

	if (buffer_available <= 0)
		return false;

	frame->addr = read_le_64(reader);
	frame->line_no = read_le_64(reader);
	frame->offset = read_le_64(reader);
	if (reader->data.overflow == true)
		goto error;

	length = safe_strlen(reader);
	if (reader->data.overflow == true)
		goto error;
	frame->symbol = reader->data.cursor;
	reader->data.cursor += length + 1;

	length = safe_strlen(reader);
	if (reader->data.overflow == true)
		goto error;
	frame->filename = reader->data.cursor;
	reader->data.cursor += length + 1;

	frame->register_count = read_le_16(reader);
	if (reader->data.overflow == true)
		goto error;

	if (is_safe_access(&reader->data, frame->register_count * REGISTER_SIZE)
	    == false) {
		reader->data.overflow = true;
		goto error;
	}

	if (frame->register_count > 0) {
		frame->register_data = (uint8_t *)reader->data.cursor;
		frame->register_buffer_size = frame->register_count * REGISTER_SIZE;
		reader->data.cursor += frame->register_buffer_size;
	}

	return true;

error:
	reader->data.cursor = initial_cursor_value;
	return false;
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
	size_t max_registers = 0;
	char *buffer;

	if (frame->register_buffer_size > 0)
		max_registers = frame->register_buffer_size / REGISTER_SIZE;

	if (frame->register_count >= max_registers)
		return false;

	buffer = frame->register_data + REGISTER_SIZE * frame->register_count;

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
	uint8_t *buffer;

	if (index >= frame->register_count)
		return false;

	buffer = frame->register_data + REGISTER_SIZE * index;
	*reg = 0;
	memcpy(reg, buffer, sizeof(uint16_t));

	buffer += sizeof(uint16_t);
	*value = 0;
	memcpy(value, buffer, sizeof(uint64_t));

	return true;
}

static bool
is_safe_access(const struct bun_writer_reader_base *base, size_t bytes)
{
	char *const end_cursor = base->buffer + base->size;
	size_t bytes_left = end_cursor - base->cursor;

	if (base->overflow == true)
		return false;

	return bytes <= bytes_left;
}

static uint16_t
read_le_16(struct bun_reader *src)
{
	uint16_t value;

	if (is_safe_access(&src->data, sizeof(value)) == false) {
		src->data.overflow = true;
		return 0;
	}

	memcpy(&value, src->data.cursor, sizeof(value));
	src->data.cursor += sizeof(value);
	return le16toh(value);
}

static void
write_le_16(struct bun_writer *dest, uint16_t value)
{
	uint16_t le_value = htole16(value);

	if (is_safe_access(&dest->data, sizeof(value)) == false) {
		dest->data.overflow = true;
		return;
	}


	memcpy(dest->data.cursor, &le_value, sizeof(le_value));
	dest->data.cursor += sizeof(le_value);
	return;
}

static uint64_t
read_le_64(struct bun_reader *src)
{
	uint64_t value;

	if (is_safe_access(&src->data, sizeof(value)) == false) {
		src->data.overflow = true;
		return 0;
	}

	memcpy(&value, src->data.cursor, sizeof(value));
	src->data.cursor += sizeof(value);
	return le64toh(value);
}

static void
write_le_64(struct bun_writer *dest, uint64_t value)
{
	uint64_t le_value = htole64(value);

	if (is_safe_access(&dest->data, sizeof(value)) == false) {
		dest->data.overflow = true;
		return;
	}

	memcpy(dest->data.cursor, &le_value, sizeof(le_value));
	dest->data.cursor += sizeof(le_value);
	return;
}


/*
 * Safe string length computation.
 *
 * Returns length of the string or -1 if the string does not end within the
 * read buffer. In the latter case, the overflow flag is set.
 */
static ssize_t
safe_strlen(struct bun_reader *src)
{
	char *const end_cursor = src->data.buffer + src->data.size;
	const size_t max_length = end_cursor - src->data.cursor;
	size_t len;

	if (src->data.overflow == true)
		return -1;

	len = strnlen(src->data.cursor, max_length);
	if (len == max_length) {
		src->data.overflow = true;
		return -1;
	}

	return len;
}

void
bun_reader_print(struct bun_reader *reader, FILE *output)
{
	struct bun_frame frame;

	while (bun_frame_read(reader, &frame) != false) {
		fprintf(output, "Frame: %s\n", frame.symbol);
		fprintf(output, "  PC: %p\n", (void *)frame.addr);
		fprintf(output, "  Registers: %zu\n", frame.register_count);
		for (size_t i = 0; i < frame.register_count; i++) {
			enum bun_register reg;
			uintmax_t val;
			const char *str;
			bun_frame_register_get(&frame, i, &reg, &val);
			str = bun_register_to_string(reg);
			fprintf(output, "    Register %s(%04X): %lX\n", str,
			    reg, val);
		}
	}
}
