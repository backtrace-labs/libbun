list(APPEND BUNWIND_SOURCES
    ../include/bun/bun.h
    ../include/bun/stream.h
    bun_structures.h
    bun.c
    bun_signal.c
    bun_stream.c
)

add_library(bun STATIC)
target_compile_features(bun PRIVATE c_std_11)
target_include_directories(bun PUBLIC ../include)
target_include_directories(bun PRIVATE .)

target_sources(bun PRIVATE ${BUNWIND_SOURCES})
