list(APPEND BUNWIND_SOURCES
    ../include/bun/bun.h
    ../include/bun/stream.h
    bun.c
    bun_stream.c
)

add_library(bun STATIC)
target_compile_features(bun PRIVATE c_std_11)
target_include_directories(bun PUBLIC ../include)
target_link_libraries(bun PUBLIC pthread)

target_compile_definitions(bun PUBLIC BUN_DETECTED_SYSTEM_BACKEND=BUN_BACKEND_NONE)

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
