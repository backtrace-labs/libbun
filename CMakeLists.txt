cmake_minimum_required(VERSION 3.5)

project(backtrace-libbun
    VERSION 0.1
    LANGUAGES C CXX
)

if(ANDROID)
    add_subdirectory("external/libunwindstack-ndk/cmake/")
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    find_package(Threads REQUIRED)
    if (NOT ANDROID)
        find_package(GTest REQUIRED)
    else()
        set(GOOGLETEST_ROOT ${ANDROID_NDK}/sources/third_party/googletest)
        add_library(gtest STATIC ${GOOGLETEST_ROOT}/src/gtest_main.cc ${GOOGLETEST_ROOT}/src/gtest-all.cc)
        target_include_directories(gtest PRIVATE ${GOOGLETEST_ROOT})
        target_include_directories(gtest PUBLIC ${GOOGLETEST_ROOT}/include)
    endif()
    add_subdirectory(tests)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
