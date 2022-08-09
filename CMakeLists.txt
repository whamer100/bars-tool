cmake_minimum_required(VERSION 3.22)
project(bars-tool)

set(CMAKE_CXX_STANDARD 20)

# zstd
set(ZSTD_BUILD_TEST OFF)
add_subdirectory(vendor/zstd/build/cmake)
set(zstd_header vendor/zstd/lib/zstd.h)
include_directories(vendor/zstd/lib/)

# argparse
add_subdirectory(vendor/argparse)
set(argparse_header vendor/argparse/include/argparse/argparse.hpp)
include_directories(vendor/argparse/include)

# styler
add_subdirectory(vendor/styler)
set(styler_header vendor/styler/include/styler.hpp)
include_directories(vendor/styler/include)

set(vendor_sources ${zstd_header} ${argparse_header} ${styler_header})

# bars-tool
set(bars_sources handlers.h barstypes.h macros.h logging.h crc32.h)

add_executable(bars-tool main.cpp ${vendor_sources} ${bars_sources})
target_link_libraries(bars-tool libzstd_static)
