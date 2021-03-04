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
    struct bun_buffer buffer;
    bool flag = false;

    auto unwind = [&](auto &&...) -> size_t {
        flag = true;
        return 100;
    };

    bool init_result = initialize_test_backend(&handle, unwind, [](auto){});
    ASSERT_TRUE(init_result);

    bool buffer_init_result = bun_buffer_init(&buffer, buf, sizeof(buf));
    ASSERT_TRUE(buffer_init_result);

    ASSERT_FALSE(flag);

    ASSERT_GT(bun_unwind(&handle, &buffer), 0);

    ASSERT_TRUE(flag);
    bun_handle_deinit(&handle);
}

TEST(base, call_destructor) {
    struct bun_handle handle;
    char buf[256];
    struct bun_buffer buffer;

    bool flag = false;
    auto unwind = [](auto &&...) -> size_t { return 1; };
    auto destroy = [&](auto) { flag = true; };

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    bool buffer_init_result = bun_buffer_init(&buffer, buf, sizeof(buf));
    ASSERT_TRUE(buffer_init_result);

    ASSERT_FALSE(flag);

    ASSERT_GT(bun_unwind(&handle, &buffer), 0);

    ASSERT_FALSE(flag);

    bun_handle_deinit(&handle);

    ASSERT_TRUE(flag);
}

TEST(base, write_frame) {
    struct bun_handle handle;
    std::vector<char> buf(1024);
    struct bun_buffer buffer;
    static constexpr const char *needle = "a_really_unique_test_symbol";

    auto unwind = [&](bun_handle *h, struct bun_buffer *buffer) -> size_t {
        struct bun_writer writer;
        struct bun_frame frame = {};

        bun_writer_init(&writer, buffer, BUN_ARCH_DETECTED, &handle);
        frame.line_no = 42;
        frame.symbol = needle;
        frame.symbol_length = strlen(frame.symbol);

        return bun_frame_write(&writer, &frame);
    };

    bool init_result = initialize_test_backend(&handle, unwind, [](auto){});
    ASSERT_TRUE(init_result);

    bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
    ASSERT_TRUE(buffer_init_result);

    ASSERT_GT(bun_unwind(&handle, &buffer), 0);

    ASSERT_NE(memmem(buf.data(), buf.size(), needle, strlen(needle)+1), nullptr);

    bun_handle_deinit(&handle);
}

TEST(base, two_writers_initialize)
{
    struct bun_handle handle;
    std::vector<char> buf(1024);
    struct bun_buffer buffer;
    auto unwind = [](auto &&...) -> size_t { return 1; };
    auto destroy = [&](auto){};

    bool handle_init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(handle_init_result);

    bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
    ASSERT_TRUE(buffer_init_result);

    bun_writer_t writer1, writer2;

    bool writer_1_initialized = bun_writer_init(&writer1, &buffer,
        BUN_ARCH_DETECTED, &handle);
    ASSERT_TRUE(writer_1_initialized);

    bool writer_2_initialized = bun_writer_init(&writer2, &buffer,
        BUN_ARCH_DETECTED, &handle);
    ASSERT_TRUE(writer_2_initialized);

    bun_handle_deinit(&handle);
}

TEST(base, write_once_writer_wont_initialize)
{
    struct bun_handle handle;
    std::vector<char> buf(1024);
    struct bun_buffer buffer;
    auto unwind = [](auto &&...) -> size_t { return 1; };
    auto destroy = [&](auto){};

    bool handle_init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(handle_init_result);

    handle.flags |= BUN_HANDLE_WRITE_ONCE;

    bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
    ASSERT_TRUE(buffer_init_result);

    bun_writer_t writer1, writer2;

    bool writer_1_initialized = bun_writer_init(&writer1, &buffer,
        BUN_ARCH_DETECTED, &handle);
    ASSERT_TRUE(writer_1_initialized);

    bool writer_2_initialized = bun_writer_init(&writer2, &buffer,
        BUN_ARCH_DETECTED, &handle);
    ASSERT_FALSE(writer_2_initialized);
}
