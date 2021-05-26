#include "gtest/gtest.h"

#include <bun/bun.h>
#include <bun/stream.h>

TEST(libbacktrace_poison_malloc, baseline)
{
    auto ptr = malloc(1);
    ASSERT_NE(ptr, nullptr);
    free(ptr);
}

/*
 * Exposition only. This test *will* unrecoverably crash.
 */
TEST(libbacktrace_poison_malloc, DISABLED_crash)
{
    raise(SIGPWR);
    auto ptr = malloc(1);
    ASSERT_NE(ptr, nullptr);
    free(ptr);
}

TEST(libbacktrace_poison_malloc, no_malloc_calls_in_unwind) {
    std::vector<char> buf(0x10000);
    struct bun_handle handle;
    struct bun_buffer buffer;

    ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBBACKTRACE));

    bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
    ASSERT_TRUE(buffer_init_result);

    size_t size = 0;
    raise(SIGPWR);
    size = bun_unwind(&handle, &buffer);
    raise(SIGPWR);

    ASSERT_NE(size, 0);

    bun_handle_deinit(&handle);
}
