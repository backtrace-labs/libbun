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

target_sources(bun PRIVATE ${BUNWIND_SOURCES})
