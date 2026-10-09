// Copyright 2023-2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0
#include "TestArray2D.h"
#ifndef _MSC_VER
// The std::atomic<float/double> specializations are disabled on MSVC
// (collision with the MSVC STL's own definitions).
#include "TestAtomicFloat.h"
#endif
#include "TestFiles.h"
#include "TestMemPool.h"
#ifdef PLATFORM_LINUX
#include "TestProcCpuAffinity.h"
#endif
#include "TestStrUtil.h"
#include "TestThreadPoolExecutor.h"
#include "test_util.h"

#include <scene_rdl2/pdevunit/pdevunit.h>

#include <iostream>

int
main(int argc, char *argv[])
{
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::pbr::TestArray2D);
#ifndef _MSC_VER
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::pbr::TestAtomicFloat);
#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::util::unittest::TestFiles);
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::alloc::TestMemPool); // 21.0211 sec on cobaltcard @ Apr/28/2025
#ifdef PLATFORM_LINUX // CPU affinity control is Linux-only
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::affinity::unittest::TestProcCpuAffinity);
#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::threadPoolExecutor::unittest::TestThreadPoolExecutor);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestCommonUtil); // 65.3025 sec on cobaltcard @ Apr/28/2025
    CPPUNIT_TEST_SUITE_REGISTRATION(scene_rdl2::str_util::TestStrUtil);

    return pdevunit::run(argc, argv);    
}
