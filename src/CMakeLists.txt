list(APPEND BUNWIND_SOURCES
../include/bun/bun.h
bun.c
bun_signal.c
)

add_library(bun STATIC)
target_compile_features(bun PRIVATE c_std_11)
target_include_directories(bun PUBLIC ../include)

#libbacktrace
# TEMPORARY UNTIL FindBacktrace.cmake IS CREATED
if(LIBBACKTRACE_ENABLED)
    set(LIBBACKTRACE_FOUND TRUE)
    target_compile_definitions(bun PUBLIC BUN_LIBBACKTRACE_ENABLED)
    target_include_directories(bun PRIVATE /home/pzakrzewski/csu)
    target_link_libraries(bun PUBLIC backtrace)
    list(APPEND BUNWIND_SOURCES
        backend/libbacktrace/bun_libbacktrace.c
        backend/libbacktrace/bun_libbacktrace.h
    )
endif()

#libunwind
# TEMPORARY UNTIL FindUnwind.cmake IS CREATED
if(LIBUNWIND_ENABLED)
    set(LIBUNWIND_FOUND TRUE)
    target_compile_definitions(bun PUBLIC BUN_LIBUNWIND_ENABLED)
    target_include_directories(bun PRIVATE /home/pzakrzewski/csu)
    target_link_libraries(bun PUBLIC unwind)
    list(APPEND BUNWIND_SOURCES
        backend/libunwind/bun_libunwind.h
        backend/libunwind/bun_libunwind.c
    )
endif()

target_sources(bun PRIVATE ${BUNWIND_SOURCES})
