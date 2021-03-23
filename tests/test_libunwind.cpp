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

TEST(libunwind, initialize) {
    char buf[128] = {};

    bun_config cfg;
    cfg.unwind_backend = BUN_LIBUNWIND;
    cfg.buffer_size = sizeof(buf);
    cfg.buffer = buf;

    bun_t *handle = bun_create(&cfg);
    ASSERT_TRUE(handle);
}
