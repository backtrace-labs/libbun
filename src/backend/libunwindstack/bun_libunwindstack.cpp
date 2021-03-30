#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bun_libunwindstack.h"
#include "../../bun_internal.h"

#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsGetLocal.h>
#include <unwindstack/Unwinder.h>

extern "C"
bun_t *_bun_initialize_libunwindstack(struct bun_config *config)
{
    (void)config;
    return NULL;
}

size_t libbacktrace_unwind(void *ctx, void *dest, size_t buf_size)
{
    (void)ctx;
    assert(ctx == NULL);

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

    auto hdr = static_cast<bun_payload_header *>(dest);
    hdr->architecture = BUN_ARCH_X86_64;
    hdr->version = 1;

    bun_frame *frame_ptr = reinterpret_cast<bun_frame *>(static_cast<char *>(dest) +
        sizeof(bun_payload_header));
    char *end_of_buffer = static_cast<char *>(dest) + buf_size;

    for (const auto &frame : unwinder.frames()) {
        frame_ptr->addr = frame.function_offset;
        strncpy(frame_ptr->symbol, frame.function_name.c_str(),
            sizeof(frame_ptr->symbol)-1);
        *std::rbegin(frame_ptr->symbol) = '\0';
        strncpy(frame_ptr->filename, frame.map_name.c_str(),
            sizeof(frame_ptr->filename)-1);
        *std::rbegin(frame_ptr->filename) = '\0';
        frame_ptr->line_no = frame.function_offset;
        frame_ptr++;
        auto offset = reinterpret_cast<char *>(frame_ptr) -
            static_cast<char *>(dest);
        if (offset >= buf_size)
            break;
    }

    size_t written = reinterpret_cast<char *>(frame_ptr) -
        static_cast<char *>(dest);

    return written;
}
