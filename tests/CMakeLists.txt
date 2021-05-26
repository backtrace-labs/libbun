add_library(test_helper STATIC helper/bun_signal.c)
target_include_directories(test_helper PUBLIC helper)
target_link_libraries(test_helper PUBLIC bun)

set(TEST_LIBRARIES
    bun gtest gtest_main Threads::Threads test_helper
)

add_executable(test_base test_base.cpp)
target_link_libraries(test_base ${TEST_LIBRARIES})
add_test(NAME base COMMAND test_base)

add_executable(test_signal test_signal.cpp)
target_link_libraries(test_signal ${TEST_LIBRARIES})
add_test(NAME signal COMMAND test_signal)

if (LIBUNWIND_ENABLED)
    add_executable(test_libunwind test_libunwind.cpp)
    target_link_libraries(test_libunwind ${TEST_LIBRARIES})
    add_test(NAME libunwind COMMAND test_libunwind)
endif()

if (LIBBACKTRACE_ENABLED)
    add_executable(test_libbacktrace test_libbacktrace.cpp)
    target_link_libraries(test_libbacktrace ${TEST_LIBRARIES})
    add_test(NAME libbacktrace COMMAND test_libbacktrace)
endif()
