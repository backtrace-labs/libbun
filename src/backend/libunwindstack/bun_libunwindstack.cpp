#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bun/stream.h>

#include "bun_libunwindstack.h"

#include "../../bun_internal.h"

#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsGetLocal.h>
#include <unwindstack/Unwinder.h>

static size_t libunwindstack_unwind(struct bun_handle *, struct bun_buffer *);

static void
destroy_handle(struct bun_handle *)
{

    return;
}

extern "C"
bool bun_internal_initialize_libunwindstack(struct bun_handle *handle)
{

    handle->unwind = libunwindstack_unwind;
    handle->destroy = destroy_handle;
    return handle;
}

void
libunwindstack_populate_regs_arm64(struct bun_frame *frame,
    unwindstack::Regs &registers)
{
    auto& regsImpl = dynamic_cast<unwindstack::RegsImpl<unsigned long>&>(registers);

    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X0, (regsImpl)[0]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X1, (regsImpl)[1]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X2, (regsImpl)[2]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X3, (regsImpl)[3]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X4, (regsImpl)[4]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X5, (regsImpl)[5]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X6, (regsImpl)[6]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X7, (regsImpl)[7]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X8, (regsImpl)[8]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X9, (regsImpl)[9]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X10, (regsImpl)[10]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X11, (regsImpl)[11]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X12, (regsImpl)[12]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X13, (regsImpl)[13]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X14, (regsImpl)[14]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X15, (regsImpl)[15]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X16, (regsImpl)[16]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X17, (regsImpl)[17]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X18, (regsImpl)[18]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X19, (regsImpl)[19]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X20, (regsImpl)[20]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X21, (regsImpl)[21]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X22, (regsImpl)[22]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X23, (regsImpl)[23]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X24, (regsImpl)[24]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X25, (regsImpl)[25]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X26, (regsImpl)[26]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X27, (regsImpl)[27]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X28, (regsImpl)[28]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X29, (regsImpl)[29]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X30, (regsImpl)[30]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_X31, (regsImpl)[31]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_PC, (regsImpl)[32]);
    bun_frame_register_append(frame, BUN_REGISTER_AARCH64_PSTATE, (regsImpl)[33]);
}

void
libunwindstack_populate_regs_arm(struct bun_frame *frame,
    unwindstack::Regs &registers)
{
    auto& regsImpl = dynamic_cast<unwindstack::RegsImpl<unsigned int>&>(registers);

    bun_frame_register_append(frame, BUN_REGISTER_ARM_R0, (regsImpl)[0]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R1, (regsImpl)[1]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R2, (regsImpl)[2]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R3, (regsImpl)[3]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R4, (regsImpl)[4]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R5, (regsImpl)[5]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R6, (regsImpl)[6]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R7, (regsImpl)[7]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R8, (regsImpl)[8]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R9, (regsImpl)[9]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R10, (regsImpl)[10]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R11, (regsImpl)[11]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R12, (regsImpl)[12]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R13, (regsImpl)[13]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R14, (regsImpl)[14]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_R15, (regsImpl)[15]);
    bun_frame_register_append(frame, BUN_REGISTER_ARM_PSTATE, (regsImpl)[16]);
}

void
libunwindstack_populate_regs_x86_64(struct bun_frame *frame,
    unwindstack::Regs &registers)
{
    auto& regsImpl = dynamic_cast<unwindstack::RegsImpl<unsigned long>&>(registers);

    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RAX, (regsImpl)[0]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RDX, (regsImpl)[1]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RCX, (regsImpl)[2]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RBX, (regsImpl)[3]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RSI, (regsImpl)[4]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RDI, (regsImpl)[5]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RBP, (regsImpl)[6]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RSP, (regsImpl)[7]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R8, (regsImpl)[8]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R9, (regsImpl)[9]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R10, (regsImpl)[10]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R11, (regsImpl)[11]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R12, (regsImpl)[12]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R13, (regsImpl)[13]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R14, (regsImpl)[14]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_R15, (regsImpl)[15]);
    bun_frame_register_append(frame, BUN_REGISTER_X86_64_RIP, (regsImpl)[16]);
}

void libunwindstack_populate_regs(struct bun_frame *frame,
    unwindstack::Regs &registers)
{
#ifdef __x86_64__
    libunwindstack_populate_regs_x86_64(frame, registers);
#elif __i386__
    // No support for x86, we don't support Crashpad x86 either
#elif __aarch64__
    libunwindstack_populate_regs_arm64(frame, registers);
#elif __arm__
    libunwindstack_populate_regs_arm(frame, registers);
#endif
}

size_t libunwindstack_unwind(struct bun_handle *handle,
    struct bun_buffer *buffer)
{
    auto *hdr = static_cast<struct bun_payload_header *>(bun_buffer_payload(buffer));
    bun_writer_t writer;

    bun_writer_init(&writer, buffer, BUN_ARCH_DETECTED, handle);

    bun_header_backend_set(&writer, BUN_BACKEND_LIBUNWINDSTACK);

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

        // 10 bytes per register (2 for enum and 8 for value), 34 registers in the worst case (arm64)
        uint8_t register_buf[340];
        bun_frame.register_buffer_size = sizeof(register_buf);
        bun_frame.register_data = register_buf;

        libunwindstack_populate_regs(&bun_frame, *registers);

        bun_header_tid_set(&writer, gettid());

        bun_frame_write(&writer, &bun_frame);
    }

    return hdr->size;
}
