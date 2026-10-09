// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "pdevunit.h"

#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestListener.h>
#include <cstdlib> // std::getenv
#if defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // SetErrorMode
#include <crtdbg.h>  // _CrtSetReportMode
#endif
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/BriefTestProgressListener.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#if !defined(_WIN32)
// Neither header is actually used by this file; kept for POSIX builds only.
#include <unistd.h>
#include <getopt.h>
#endif
#include <cstring>



namespace {

// With PDEVUNIT_VERBOSE set, print each test name (unbuffered) before it
// runs — indispensable when a test crashes the process and the buffered
// stdout report is lost.
class VerboseListener : public CppUnit::TestListener
{
public:
    void startTest(CppUnit::Test* test) override
    {
        std::cerr << "[test] " << test->getName() << std::endl;
    }
};

} // namespace

int
pdevunit::run(int argc, char* argv[])
{
#if defined(_MSC_VER)
    // Batch/CI friendliness: route CRT assertions and heap reports to
    // stderr instead of interactive dialog boxes (which hang headless runs),
    // and suppress the WER fault dialog.
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

    char* tests=strdup("");

    try {
        CppUnit::TestFactoryRegistry& registry =
             CppUnit::TestFactoryRegistry::getRegistry();

        CppUnit::TestRunner runner;
        runner.addTest(registry.makeTest());

        CppUnit::TestResult controller;

        // Add results collector

        CppUnit::TestResultCollector result;
        controller.addListener(&result);

        VerboseListener verbose;
        if (std::getenv("PDEVUNIT_VERBOSE")) {
            controller.addListener(&verbose);
        }
        
         // Run the tests
        
        runner.run(controller, tests);

        // Pass the results to the appropriate outputter.  
        
        CppUnit::CompilerOutputter outputter(&result, std::cerr);
        outputter.write();

       
        return result.wasSuccessful() ? EXIT_SUCCESS : EXIT_FAILURE;
        
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}


// --------------------------------------------------------------------------- 

