add_library(bcd STATIC bcd-amalgamated.c include/bcd.h)
target_include_directories(bcd PUBLIC include)

if(ANDROID)
    target_compile_definitions(bcd PRIVATE BCD_NDK_BUILD)
endif()
