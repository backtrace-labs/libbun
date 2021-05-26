set(HELPER_SOURCES
    helper/bun_signal.c
    helper/payload_header.c
    helper/payload_header.h
    helper/test_backend.hpp
)

add_subdirectory(helper/poison_malloc)

add_library(test_helper STATIC ${HELPER_SOURCES})
target_include_directories(test_helper PUBLIC helper)
target_include_directories(test_helper PUBLIC ../src)
target_link_libraries(test_helper PUBLIC bun)

set(TEST_LIBRARIES
    bun gtest Threads::Threads test_helper
)

if (ANDROID)
    list(APPEND TEST_LIBRARIES log)
else()
    list(APPEND TEST_LIBRARIES gtest_main)
endif()

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

    add_executable(test_libunwind_poison_malloc test_libunwind_poison_malloc.cpp)
    target_link_libraries(test_libunwind_poison_malloc ${TEST_LIBRARIES})
    target_link_libraries(test_libunwind_poison_malloc poison_malloc)
    add_test(NAME libunwind_poison_malloc COMMAND test_libunwind_poison_malloc)
endif()

if (LIBBACKTRACE_ENABLED)
    add_executable(test_libbacktrace test_libbacktrace.cpp)
    target_link_libraries(test_libbacktrace ${TEST_LIBRARIES})
    add_test(NAME libbacktrace COMMAND test_libbacktrace)

    add_executable(test_libbacktrace_poison_malloc test_libbacktrace_poison_malloc.cpp)
    target_link_libraries(test_libbacktrace_poison_malloc ${TEST_LIBRARIES})
    target_link_libraries(test_libbacktrace_poison_malloc poison_malloc)
    add_test(NAME libbacktrace_poison_malloc COMMAND test_libbacktrace_poison_malloc)
endif()

if (LIBUNWINDSTACK_ENABLED)
    add_executable(test_libunwindstack test_libunwindstack.cpp)
    target_link_libraries(test_libunwindstack ${TEST_LIBRARIES})
    add_test(NAME libunwindstack COMMAND test_libunwindstack)

    add_executable(test_libunwindstack_poison_malloc test_libunwindstack_poison_malloc.cpp)
    target_link_libraries(test_libunwindstack_poison_malloc ${TEST_LIBRARIES})
    target_link_libraries(test_libunwindstack_poison_malloc poison_malloc)
    add_test(NAME libunwindstack_poison_malloc COMMAND test_libunwindstack_poison_malloc)
endif()
