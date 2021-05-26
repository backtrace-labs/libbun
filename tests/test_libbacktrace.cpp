#include "gtest/gtest.h"

#include <algorithm>
#include <functional>
#include <vector>
#include <string>

#include <bun/bun.h>
#include <bun/stream.h>

static int dummy_line;
/*
 * This function is not marked static on purpose - we do want the name to appear
 * in the callstack
 */
void dummy_func(std::function<void()> const& f)
{
    f(); dummy_line = __LINE__;
}

TEST(libbacktrace, initialize) {
    struct bun_handle handle;
    ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBBACKTRACE));
    bun_handle_deinit(&handle);
}

TEST(libbacktrace, unwinding) {
    std::vector<char> buf(0x10000);
    struct bun_handle handle;

    ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBBACKTRACE));

    size_t size = 0;
    dummy_func([&]{ size = bun_unwind(&handle, buf.data(), buf.size()); });

    ASSERT_NE(size, 0);

    struct bun_reader reader;
    bun_reader_init(&reader, buf.data(), size);

    bun_payload_header *header = reinterpret_cast<bun_payload_header *>(buf.data());
    ASSERT_EQ(header->version, 1);

    std::vector<bun_frame> frames;
    bun_frame next_frame;

    while (bun_frame_read(&reader, &next_frame)) {
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
    ASSERT_EQ(frame.line_no, dummy_line);
    bun_handle_deinit(&handle);
}

TEST(libbacktrace, tiny_buffer)
{
    std::vector<char> buf(sizeof(bun_payload_header));
    struct bun_handle handle;

    ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBBACKTRACE));
    size_t size = 0;
    dummy_func([&]{ size = bun_unwind(&handle, buf.data(), buf.size()); });

    ASSERT_EQ(size, 0);

    bun_handle_deinit(&handle);
}
