#pragma once

#include <stdint.h>

struct bun_handle
{
    size_t(*unwind_function)(void *, void *, size_t);
    void(*free_context)(void *);
    void *unwinder_context;
    size_t unwind_buffer_size;
    void *unwind_buffer;
};

enum bun_architecture
{
    BUN_ARCH_X86,
    BUN_ARCH_X86_64,
};

struct bun_payload_header
{
    uint16_t version;
    uint16_t architecture;
    uint32_t size;
};

struct bun_frame
{
    uint64_t addr;
    char symbol[40];
    char filename[216];
};

struct bun_payload
{
    struct bun_payload_header header;
    uint32_t num_frames;
    struct bun_frame frames[0];
};