#include "gtest/gtest.h"

#include <bun/bun.h>
#include <bun/stream.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <string>

int dummy_line;
/*
 * This function is not marked static on purpose - we do want the name to appear
 * in the callstack
 */
void dummy_func(std::function<void()> const& f)
{
    f(); dummy_line = __LINE__;
}

TEST(libunwind, initialize) {
    char buf[128] = {};

    bun_config cfg;
    cfg.unwind_backend = BUN_BACKEND_LIBUNWIND;
    cfg.buffer_size = sizeof(buf);
    cfg.buffer = buf;

    bun_t *handle = bun_create(&cfg);
    ASSERT_TRUE(handle);
}

TEST(libunwind, unwinding) {
    std::vector<char> buf(0x10000);
    bun_config cfg = BUN_CONFIG_INITIALIZE;
    cfg.unwind_backend = BUN_BACKEND_LIBUNWIND;
    cfg.buffer_size = buf.size();
    cfg.buffer = buf.data();
    bun_t *handle = bun_create(&cfg);

    ASSERT_TRUE(handle);
    void *data = nullptr;
    size_t size = 0;
    dummy_func([&]{ bun_unwind(handle, &data, &size); });

    ASSERT_TRUE(data);
    ASSERT_NE(size, 0);

    bun_writer_reader *reader = bun_create_reader(data, size);

    bun_payload_header *header = reinterpret_cast<bun_payload_header *>(buf.data());
    ASSERT_NE(header, nullptr);
    ASSERT_EQ(header->version, 1);

    std::vector<bun_frame> frames;
    bun_frame next_frame;

    while (bun_frame_read(reader, &next_frame)) {
        frames.push_back(next_frame);
    }

    ASSERT_GT(frames.size(), 0);

    auto pred = [](bun_frame const& f) {
        return strcmp(f.symbol, "_Z10dummy_funcRKSt8functionIFvvEE") == 0;
    };
    auto it = std::find_if(frames.cbegin(), frames.cend(), pred);
    ASSERT_NE(it, frames.cend());
    const bun_frame& frame = *it;
    ASSERT_NE(dummy_line, 0);
    ASSERT_EQ(frame.offset, 0x18);
}
