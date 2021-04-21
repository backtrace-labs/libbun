#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bun/stream.h>

#include "bun_libunwindstack.h"

#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsGetLocal.h>
#include <unwindstack/Unwinder.h>

static size_t libunwindstack_unwind(void *ctx);

extern "C"
bun_t *_bun_initialize_libunwindstack(struct bun_config *config)
{
    bun_t *handle = static_cast<bun_t *>(calloc(1, sizeof(struct bun_handle)));

    if (handle == NULL)
        return NULL;

    handle->unwind_function = libunwindstack_unwind;
    handle->unwind_buffer = config->buffer;
    handle->unwind_buffer_size = config->buffer_size;
    handle->arch = config->arch;
    return handle;
}

size_t libunwindstack_unwind(void *ctx)
{
    struct bun_handle *handle = static_cast<bun_handle *>(ctx);
    struct bun_payload_header *hdr = static_cast<bun_payload_header *>(handle->unwind_buffer);
    struct bun_writer_reader *writer;

    writer = bun_create_writer(handle->unwind_buffer,
        handle->unwind_buffer_size, handle->arch);

    std::unique_ptr<unwindstack::Regs> registers;
    unwindstack::LocalMaps local_maps;

    registers = std::unique_ptr<unwindstack::Regs>(
        unwindstack::Regs::CreateFromLocal());
    unwindstack::RegsGetLocal(registers.get());

    if (local_maps.Parse() == false) {
        return 0;
    }

    auto process_memory = unwindstack::Memory::CreateProcessMemory(getpid());

    constexpr static size_t max_frames = 128;
    unwindstack::Unwinder unwinder{
        max_frames, &local_maps, registers.get(), process_memory
    };
    unwinder.Unwind();

    for (const auto &frame : unwinder.frames()) {
        struct bun_frame bun_frame;
        memset(&bun_frame, 0, sizeof(bun_frame));

        bun_frame.addr = frame.pc;
        bun_frame.symbol = const_cast<char *>(frame.function_name.c_str());
        bun_frame.symbol_length = frame.function_name.size();
        bun_frame.filename = const_cast<char *>(frame.map_name.c_str());
        bun_frame.filename_length = frame.map_name.size();
        bun_frame.line_no = frame.function_offset;

        bun_frame_write(writer, &bun_frame);
    }
    bun_free_writer_reader(writer);
    return hdr->size;
}
