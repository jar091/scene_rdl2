# Copyright 2023-2026 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

# These tools drive the shared-memory / NUMA / CPU-affinity side of
# common_grid_util, which is not built on Windows (see docs/WINDOWS_PORT.md).
if(NOT IsWindowsPlatform)
    add_subdirectory(affinityMapTool)
    add_subdirectory(numaInfo)
    add_subdirectory(shmFbDump)
    add_subdirectory(shmFbTool)
    add_subdirectory(shmFootmarkDump)
    add_subdirectory(shmUtilTool)
endif()
add_subdirectory(snapshotDeltaDump)
add_subdirectory(threadPoolExecutorTest)
