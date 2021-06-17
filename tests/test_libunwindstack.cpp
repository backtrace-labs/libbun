#include "gtest/gtest.h"

#include <bun/bun.h>
#include <bun/stream.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <string>

#include "payload_header.h"

int dummy_line;
/*
 * This function is not marked static on purpose - we do want the name to appear
 * in the callstack
 */
void dummy_func(std::function<void()> const& f)
{
	f(); dummy_line = __LINE__;
}

TEST(libunwindstack, initialize) {
	struct bun_handle handle;
	ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBUNWINDSTACK));
	bun_handle_deinit(&handle);
}

TEST(libunwindstack, unwinding) {
	std::vector<char> buf(0x10000);
	struct bun_handle handle;
	struct bun_buffer buffer;

	ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBUNWINDSTACK));

	bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
	ASSERT_TRUE(buffer_init_result);

	size_t size = 0;
	dummy_func([&]{ size = bun_unwind(&handle, &buffer); });

	ASSERT_NE(size, 0);

	struct bun_reader reader;
	bun_reader_init(&reader, &buffer, &handle);

	std::vector<bun_frame> frames;
	bun_frame next_frame;

	while (bun_frame_read(&reader, &next_frame)) {
		frames.push_back(next_frame);
	}

	ASSERT_GT(frames.size(), 0);

	auto pred = [](bun_frame const& f) {
		return strcmp(f.symbol, "_Z10dummy_funcRKNSt6__ndk18functionIFvvEEE") == 0;
	};
	auto it = std::find_if(frames.cbegin(), frames.cend(), pred);
	ASSERT_NE(it, frames.cend());
	const bun_frame& frame = *it;
	ASSERT_NE(dummy_line, 0);
	bun_handle_deinit(&handle);
}

TEST(libunwindstack, tiny_buffer)
{
	std::vector<char> buf(payload_header_size());
	struct bun_handle handle;
	struct bun_buffer buffer;

	bool buffer_init_result = bun_buffer_init(&buffer, buf.data(), buf.size());
	ASSERT_TRUE(buffer_init_result);

	ASSERT_TRUE(bun_handle_init(&handle, BUN_BACKEND_LIBUNWINDSTACK));
	size_t size = 0;
	dummy_func([&]{ size = bun_unwind(&handle, &buffer); });

	ASSERT_EQ(size, 0);

	bun_handle_deinit(&handle);
}
