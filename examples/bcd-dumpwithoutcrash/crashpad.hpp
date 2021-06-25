#pragma once

#include <signal.h>
#include <ucontext.h>

#include <bun/bun.h>
#include <bun/stream.h>

namespace backtrace {

extern bun_handle handle;

using handler_t = bool(int, siginfo_t *, ucontext_t *);

bool initCrashpad(bun_buffer *buffer, handler_t *handler = nullptr);

void dumpWithoutCrash(ucontext_t *);

void crash();

}
