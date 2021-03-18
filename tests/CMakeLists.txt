
set(TEST_LIBRARIES
    bun gtest gtest_main Threads::Threads
)

if (LIBBACKTRACE_ENABLED)
    add_executable(test_libbacktrace test_libbacktrace.cpp)
    target_link_libraries(test_libbacktrace ${TEST_LIBRARIES})
    add_test(NAME libbacktrace COMMAND test_libbacktrace)
endif()

if (LIBUNWIND_ENABLED)
    add_executable(test_libunwind test_libunwind.cpp)
    target_link_libraries(test_libunwind ${TEST_LIBRARIES})
    add_test(NAME libunwind COMMAND test_libunwind)
endif()
