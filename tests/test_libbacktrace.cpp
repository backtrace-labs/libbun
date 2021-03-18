#include "gtest/gtest.h"

#include <bun/bun.h>
#include "../src/bun_internal.h"

#include <algorithm>
#include <functional>
#include <vector>
#include <string>

// not static on purpose
void dummy_func(std::function<void()> const& f)
{
    f();
}

TEST(libbacktrace, initialize) {
    char buf[128] = {};

    bun_config cfg;
    cfg.unwind_backend = BUN_LIBBACKTRACE;
    cfg.buffer_size = sizeof(buf);
    cfg.buffer = buf;

    bun_handle_t handle = bun_initialize(&cfg);
    ASSERT_TRUE(handle);
}

TEST(libbacktrace, unwinding) {
    std::vector<char> buf(0x10000);
    bun_config cfg;
    cfg.unwind_backend = BUN_LIBBACKTRACE;
    cfg.buffer_size = buf.size();
    cfg.buffer = buf.data();
    bun_handle_t handle = bun_initialize(&cfg);

    ASSERT_TRUE(handle);
    void *data = nullptr;
    size_t size = 0;
    dummy_func([&]{ bun_unwind(handle, &data, &size); });

    ASSERT_TRUE(data);
    ASSERT_NE(size, 0);

    bun_payload_header *header = reinterpret_cast<bun_payload_header *>(buf.data());
    ASSERT_EQ(header->version, 1);

    std::vector<bun_frame> frames;
    size_t num_frames = (header->size - sizeof(bun_payload_header)) /
        sizeof(bun_frame);
    ASSERT_EQ(num_frames * sizeof(bun_frame) + sizeof(bun_payload_header), header->size);
    frames.resize(num_frames);
    for (size_t i = 0; i < num_frames; i++) {
        size_t offset = i * sizeof(bun_frame) + sizeof(bun_payload_header);
        memcpy(&frames[i], buf.data() + offset, sizeof(bun_frame));
    }
    // fprintf(stderr, "func = %p\n", &dummy_func);
    // for(bun_frame const& f : frames) {
    //     fprintf(stderr, "%p %s %s\n", f.addr, f.filename, f.symbol);
    // }
    auto pred = [](bun_frame const& f) {
        return strcmp(f.symbol, "_Z10dummy_funcRKSt8functionIFvvEE") == 0;
    };
    auto it = std::find_if(frames.cbegin(), frames.cend(), pred);
    ASSERT_NE(it, frames.cend());
}
