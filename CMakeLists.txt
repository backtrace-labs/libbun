cmake_minimum_required(VERSION 3.10)

project(backtrace-libbun
    VERSION 0.1
    LANGUAGES C CXX
)

set(LIBBACKTRACE_ENABLED FALSE)
set(LIBUNWIND_ENABLED FALSE)
set(LIBUNWINDSTACK_ENABLED TRUE)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


if(LIBUNWINDSTACK_ENABLED)
    add_subdirectory("external/libunwindstack-ndk/cmake/")
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    find_package(Threads REQUIRED)
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
