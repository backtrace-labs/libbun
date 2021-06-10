#include <atomic>

#include "gtest/gtest.h"

#include <bun/bun.h>
#include <bun/stream.h>

#include "test_backend.hpp"
#include "bun_signal.h"

TEST(signal, initialize_backend_none) {
    struct bun_handle handle;
    ASSERT_FALSE(bun_handle_init(&handle, BUN_BACKEND_NONE));
}

TEST(signal, sigaction_set) {
    struct bun_handle handle;
    char buf[256];

    auto unwind = [&](auto &&...) -> size_t { return 1; };
    auto destroy = [](auto){};

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    auto old = signal(SIGINT, SIG_IGN);
    raise(SIGINT);

    signal(SIGINT, old);

    ASSERT_GT(bun_unwind(&handle, buf, sizeof(buf)), 0);
    bun_handle_deinit(&handle);
}

TEST(signal, call_unwinder) {
    struct bun_handle handle;
    char buf[256];
    std::atomic_bool flag{false};

    auto unwind = [&](auto &&...) -> size_t {
        flag = true;
        return 100;
    };
    auto destroy = [](auto){};

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    ASSERT_FALSE(flag);

    auto old = signal(SIGABRT, SIG_IGN);

    auto sigaction_set = bun_sigaction_set(&handle, buf, sizeof(buf));
    ASSERT_TRUE(sigaction_set);

    raise(SIGABRT);

    signal(SIGABRT, old);

    ASSERT_TRUE(flag);
    bun_handle_deinit(&handle);
}

TEST(signal, signal_in_signal) {
    struct bun_handle handle;
    char buf[256];
    std::atomic<int> counter{0};
    int signum = SIGABRT;

    auto unwind = [&](auto &&...) -> size_t {
        counter++;
        signum++;
        if (counter < 5)
            raise(signum);
        return 100;
    };
    auto destroy = [](auto){};

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    ASSERT_EQ(counter, 0);

    auto old = signal(SIGABRT, SIG_IGN);

    auto sigaction_set = bun_sigaction_set(&handle, buf, sizeof(buf));
    ASSERT_TRUE(sigaction_set);

    ASSERT_EQ(counter, 0);
    raise(SIGABRT);
    ASSERT_EQ(counter, 1);

    signal(SIGABRT, old);

    bun_handle_deinit(&handle);
}

TEST(signal, 1024_signals) {
    struct bun_handle handle;
    char buf[256];
    std::atomic<int> counter{0};

    auto unwind = [&](auto &&...) -> size_t {
        counter++;
        return 100;
    };
    auto destroy = [](auto){};

    bool init_result = initialize_test_backend(&handle, unwind, destroy);
    ASSERT_TRUE(init_result);

    ASSERT_EQ(counter, 0);

    auto old = signal(SIGABRT, SIG_IGN);

    auto sigaction_set = bun_sigaction_set(&handle, buf, sizeof(buf));
    ASSERT_TRUE(sigaction_set);

    ASSERT_EQ(counter, 0);

    for (int i = 0; i < 1024; i++) {
        ASSERT_EQ(counter, i);
        raise(SIGABRT);
        ASSERT_EQ(counter, i+1);

    }

    signal(SIGABRT, old);

    bun_handle_deinit(&handle);
}
