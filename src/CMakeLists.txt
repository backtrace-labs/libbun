list(APPEND BUNWIND_SOURCES
    ../include/bun/bun.h
    ../include/bun/stream.h
    bun.c
    bun_stream.c
)

add_library(bun STATIC)
target_compile_features(bun PRIVATE c_std_11)
target_compile_features(bun PRIVATE cxx_std_17)
target_include_directories(bun PUBLIC ../include)
#target_link_libraries(bun PUBLIC pthread)

find_package(libunwind)

if(NOT LIBUNWIND_FOUND AND LIBUNWIND_ENABLED)
    message(FATAL_ERROR "libunwind not found")
endif()

# libunwind
if(LIBUNWIND_ENABLED)
    find_package(libunwind REQUIRED)
    target_compile_definitions(bun PUBLIC BUN_LIBUNWIND_ENABLED)
    target_include_directories(bun PRIVATE ${LIBUNWIND_INCLUDE_DIR})
    target_link_libraries(bun PUBLIC ${LIBUNWIND_LIBRARIES})
    list(APPEND BUNWIND_SOURCES
        backend/libunwind/bun_libunwind.h
        backend/libunwind/bun_libunwind.c
    )
endif()

find_package(libbacktrace)

if(NOT LIBBACKTRACE_FOUND AND BUN_LIBBACKTRACE_ENABLED)
    message(FATAL_ERROR "libbacktrace not found")
endif()

# message("LIBBACKTRACE_FOUND: ${LIBBACKTRACE_FOUND}")
# libbacktrace
if(LIBBACKTRACE_FOUND)
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


if (LIBUNWIND_FOUND)
    target_compile_definitions(bun PUBLIC BUN_DETECTED_SYSTEM_BACKEND=BUN_BACKEND_LIBUNWIND)
elseif (LIBBACKTRACE_FOUND)
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
