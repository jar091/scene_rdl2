# Copyright 2023-2025 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(fb_util)
# The grid_util tests exercise the Shm*/TlSvr (shared memory + socket) side of
# the library, which is not built on Windows (see docs/WINDOWS_PORT.md).
if(${PROJECT_NAME_UPPER}_BUILD_GRID_UTIL AND NOT IsWindowsPlatform)
    add_subdirectory(grid_util)
endif()
add_subdirectory(math)
add_subdirectory(rec_time)
add_subdirectory(simd)
