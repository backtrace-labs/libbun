#include <utility>
#include <type_traits>

#include "gtest/gtest.h"

#include <bun/bun.h>
#include <bun/stream.h>

#include "test_backend.hpp"

TEST(base, initialize_backend_none) {
    struct bun_handle handle;
    ASSERT_FALSE(bun_handle_init(&handle, BUN_BACKEND_NONE));
}

TEST(base, call_unwinder) {
    struct bun_handle handle;
    char buf[256];
    bool flag = false;

    auto unwind = [&](auto &&...) -> size_t {
        flag = true;
        return 100;
    };

    bool init_result = initialize_test_backend(&handle, unwind, [](auto){});
    ASSERT_TRUE(init_result);

    ASSERT_FALSE(flag);

    ASSERT_GT(bun_unwind(&handle, buf, sizeof(buf)), 0);

    ASSERT_TRUE(flag);
    bun_handle_deinit(&handle);
}

TEST(base, call_destructor) {
    struct bun_handle handle;
    char buf[256];

    bool flag = false;
    auto unwind = [](auto &&...) -> size_t { return 1; };
    auto destroy = [&](auto) { flag = true; };

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    ASSERT_FALSE(flag);

    ASSERT_GT(bun_unwind(&handle, buf, sizeof(buf)), 0);

    ASSERT_FALSE(flag);

    bun_handle_deinit(&handle);

    ASSERT_TRUE(flag);
}

TEST(base, write_frame) {
    struct bun_handle handle;
    std::vector<char> buf(1024);
    static constexpr const char *needle = "a_really_unique_test_symbol";

    auto unwind = [&](bun_handle *h, void *buf, size_t buf_size) -> size_t {
        struct bun_writer writer;
        bun_writer_init(&writer, buf, buf_size, BUN_ARCH_DETECTED);
        struct bun_frame frame = {};
        frame.line_no = 42;
        frame.symbol = needle;
        frame.symbol_length = strlen(frame.symbol);
        size_t written = bun_frame_write(&writer, &frame);

        auto *hdr = static_cast<const struct bun_payload_header *>(buf);
        return hdr->size;
    };

    bool init_result = initialize_test_backend(&handle, unwind, [](auto){});
    ASSERT_TRUE(init_result);

    ASSERT_GT(bun_unwind(&handle, buf.data(), buf.size()), 0);

    ASSERT_NE(memmem(buf.data(), buf.size(), needle, strlen(needle)+1), nullptr);

    bun_handle_deinit(&handle);
}
