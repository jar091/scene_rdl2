# Copyright 2023-2026 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(target scenerdl2_render_util_tests)

add_executable(${target})

# CPU affinity is Linux-only (matches lib/render/util); the atomic-float
# specializations are disabled on MSVC (STL collision), so their test is too.
if(IsLinuxPlatform)
set(PlatformSpecificSources
    TestProcCpuAffinity.cc)
endif()
if(NOT IsWindowsPlatform)
    list(APPEND PlatformSpecificSources TestAtomicFloat.cc)
endif()

target_sources(${target}
    PRIVATE
        main.cc
        test_util.cc
        TestArray2D.cc
        TestFiles.cc
        TestMemPool.cc
        TestStrUtil.cc
        TestThreadPoolExecutor.cc
        ${PlatformSpecificSources}
)

if(NOT IsWindowsPlatform)
    set(PlatformThreads pthread)
endif()

target_link_libraries(${target}
    PRIVATE
        ${PlatformThreads}
        SceneRdl2::common_platform
        SceneRdl2::pdevunit
        SceneRdl2::render_util
        TBB::tbb
)

# Set standard compile/link options
SceneRdl2_cxx_compile_definitions(${target})
SceneRdl2_cxx_compile_features(${target})
SceneRdl2_cxx_compile_options(${target})
SceneRdl2_link_options(${target})

if(${TBB_VERSION} VERSION_GREATER_EQUAL "2021.0")
    target_compile_definitions(${target} PRIVATE -DTBB_ONEAPI=1)
endif()

add_test(NAME ${target} COMMAND ${target})
set_tests_properties(${target} PROPERTIES
    LABELS "unit"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>
    # TestMemPool performs ~33M allocations: minutes in an optimized build,
    # far longer with a debug allocator. CTest's default 1500s is not enough.
    TIMEOUT 5400
)
