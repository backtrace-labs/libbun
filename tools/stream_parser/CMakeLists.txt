add_executable(bun_parse_stream main.c)

list(APPEND PARSE_STREAM_SOURCES
    main.c
    register_to_string.c
)

target_include_directories(bun_parse_stream PRIVATE .)
target_compile_features(bun_parse_stream PRIVATE c_std_11)
target_sources(bun_parse_stream PRIVATE ${PARSE_STREAM_SOURCES})
target_link_libraries(bun_parse_stream bun)
