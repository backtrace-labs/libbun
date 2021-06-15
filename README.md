# Backtrace's unwinding support library

# Prerequisites

To compile this library you need the following:
 - CMake 3.5 or newer
 - a C11-compatible compiler
 - Google Test (for testing)

On Ubuntu, the following packages are required:
 - `libgtest-dev` - for tests
 - `googletest` - for tests
 - `libunwind-dev` - for the libunwind backend

Additionally, the following backends are supported:
 - libunwind
 - libbacktrace
 - libunwindstack

# Build and test

This section assumes we're in a build folder one level below project root.

## Checkout and the build directory

```sh
git clone 'https://github.com/backtrace-labs/libbun.git'
cd libbun
mkdir build
cd build
```

## Configure with CMake

```sh
cmake \
    -DBUILD_TESTING=ON \
    -DCMAKE_MODULE_PATH=$(pwd)/../cmake \
    ..
```

Additional parameters are listed below. All of them are optional.
- `-DCRASHPAD_DIR=/path/to/crashpad` - path to non-standard location for crashpad (e.g. source build)
- `-DBUILD_TESTING=[ON|OFF]` - build tests
- `-DBUILD_TOOLS=[ON|OFF]` - build tools
- `-DBUILD_EXAMPLES=[ON|OFF]` - build examples
- `-DLIBUNWIND_ENABLED=[ON|OFF]` - build libunwind backend (default: detect if available)
- `-DLIBUNWIND_DIR=/non/standard/libunwind/dir` - path to non-standard location for crashpad (e.g. source build)
- `-DLIBBACKTRACE_ENABLED=[ON|OFF]` -  build libbacktrace backend (default: detect if available)
- `-DLIBBACKTRACE_DIR=/non/standard/libbacktrace/dir`  - path to non-standard location for libbacktrace (e.g. source build)
- `-DLIBUNWINDSTACK_ENABLED=OFF` - build libunwindstack backend (Android only, default: detect if available)

## Build with CMake

```sh
cmake --build .
```

If your CMake installation is new enough:

```sh
cmake --build . -j
```

## Test

```sh
ctest
````

For verbose and colorful output:
```sh
GTEST_COLOR=1 ctest -V
```
