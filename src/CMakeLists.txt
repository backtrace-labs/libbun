include(CheckFunctionExists)

list(APPEND BUNWIND_SOURCES
    ../include/bun/bun.h
    ../include/bun/stream.h
    ../include/bun/utils.h
    bun_internal.h
    bun.c
    bun_stream.c
    bun_utils.c
    bun_cpp_utils.cpp
    register_to_string.h
    register_to_string.c
)

check_function_exists(bun_memfd_create HAS_MEMFD_CREATE)

add_library(bun STATIC bun.c)
set_property(TARGET bun PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(bun PRIVATE c_std_11)
target_compile_features(bun PRIVATE cxx_std_17)
target_include_directories(bun PUBLIC ../include)
target_link_libraries(bun PUBLIC bcd dl)

if(NOT ANDROID)
    target_link_libraries(bun PUBLIC pthread)
endif()

find_package(Crashpad)
if (CRASHPAD_FOUND)
    add_subdirectory(bt-crashpad)
endif()

# libunwind
if(LIBUNWIND_ENABLED)
    find_package(libunwind REQUIRED)
    target_compile_definitions(bun PUBLIC BUN_LIBUNWIND_ENABLED)
    target_include_directories(bun PRIVATE ${LIBUNWIND_INCLUDE_DIR})
    if (NOT ANDROID)
        target_link_libraries(bun PUBLIC unwind-ptrace)


        if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
            target_link_libraries(bun PUBLIC unwind-x86_64)
        elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
            target_link_libraries(bun PUBLIC unwind-x86)
        elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
            target_link_libraries(bun PUBLIC unwind-arm)
        elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
            target_link_libraries(bun PUBLIC unwind-aarch64)
        endif()
    endif()


    target_link_libraries(bun PUBLIC ${LIBUNWIND_LIBRARIES})
    list(APPEND BUNWIND_SOURCES
        backend/libunwind/bun_libunwind.h
        backend/libunwind/bun_libunwind.c
    )
endif()

# libbacktrace
if(LIBBACKTRACE_ENABLED)
    target_compile_definitions(bun PUBLIC BUN_LIBBACKTRACE_ENABLED)
    target_include_directories(bun PRIVATE ${LIBBACKTRACE_INCLUDE_DIR})
    target_link_libraries(bun PUBLIC ${LIBBACKTRACE_LIBRARIES})
    list(APPEND BUNWIND_SOURCES
        backend/libbacktrace/bun_libbacktrace.c
        backend/libbacktrace/bun_libbacktrace.h
    )
endif()

if(LIBUNWINDSTACK_ENABLED)
    target_include_directories(bun PRIVATE ../external/libunwindstack-ndk/include)
    target_compile_definitions(bun PUBLIC BUN_LIBUNWINDSTACK_ENABLED)
    target_link_libraries(bun PRIVATE unwindstack)
    list(APPEND BUNWIND_SOURCES
        backend/libunwindstack/bun_libunwindstack.cpp
        backend/libunwindstack/bun_libunwindstack.h
    )
endif()

if (LIBUNWIND_ENABLED)
    target_compile_definitions(bun PUBLIC BUN_DETECTED_SYSTEM_BACKEND=BUN_BACKEND_LIBUNWIND)
elseif (LIBBACKTRACE_ENABLED)
    target_compile_definitions(bun PUBLIC BUN_DETECTED_SYSTEM_BACKEND=BUN_BACKEND_LIBBACKTRACE)
else()
    target_compile_definitions(bun PUBLIC BUN_DETECTED_SYSTEM_BACKEND=BUN_BACKEND_NONE)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    target_compile_definitions(bun PUBLIC BUN_ARCH_DETECTED=BUN_ARCH_X86_64)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
    target_compile_definitions(bun PUBLIC BUN_ARCH_DETECTED=BUN_ARCH_X86)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
    target_compile_definitions(bun PUBLIC BUN_ARCH_DETECTED=BUN_ARCH_ARM)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
    target_compile_definitions(bun PUBLIC BUN_ARCH_DETECTED=BUN_ARCH_ARM64)
else()
    target_compile_definitions(bun PUBLIC BUN_ARCH_DETECTED=BUN_ARCH_UNKNOWN)
endif()
target_sources(bun PRIVATE ${BUNWIND_SOURCES})

if (HAS_MEMFD_CREATE)
    target_compile_definitions(bun PRIVATE BUN_MEMFD_CREATE_AVAILABLE)
endif()
