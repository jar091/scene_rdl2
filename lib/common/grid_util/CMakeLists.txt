# Copyright 2023-2026 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(component common_grid_util)

set(installIncludeDir ${PACKAGE_NAME}/common/grid_util)
set(exportGroup ${PROJECT_NAME}Targets)

add_library(${component} SHARED "")
add_library(${PROJECT_NAME}::${component} ALIAS ${component})

# Sources and public headers that need POSIX sockets, System V shared memory,
# libnuma or CPU-affinity APIs. Windows has none of them and gets the portable
# subset of the library (see docs/WINDOWS_PORT.md); Darwin does have them, and
# the files that differ there carry their own PLATFORM_APPLE branches, so it
# builds the full set exactly as Linux does. The gate has to agree with the
# `#if !defined(_WIN32)` in Arg.cc, which calls TlSvr::send().
set(PlatformSpecificSources)
set(PlatformSpecificPublicHeaders)
if(NOT IsWindowsPlatform)
    set(PlatformSpecificSources
        AffinityMapTable.cc         # <pwd.h>, <sys/ipc.h>, Pipe/Process
        AffinityResourceControl.cc  # NUMA + CPU affinity
        CpuSocketUtil.cc            # /proc CPU topology
        DebugConsoleDriver.cc       # TlSvr (sockets)
        NumaUtil.cc                 # <sys/mman.h>, libnuma, syscall()
        Pipe.cc                     # popen/pclose
        Process.cc                  # <sys/types.h> pid_t, signal()
        ShmAffinityInfo.cc          # <sys/shm.h>
        ShmData.cc                  # <sys/shm.h>, <pwd.h>
        ShmFb.cc                    # ShmData
        ShmFbOutput.cc              # ShmFb + TlSvr
        ShmFootmark.cc              # <sys/shm.h>, <sys/time.h>
        SockUtil.cc                 # <sys/socket.h>
        TlSvr.cc                    # <sys/socket.h>, <netinet/in.h>
        UserUtil.cc                 # <pwd.h>, getuid()
    )
    set(PlatformSpecificPublicHeaders
        AffinityResourceControl.h
        CpuSocketUtil.h
        DebugConsoleDriver.h
        NumaUtil.h
        ShmData.h
        ShmFb.h
        ShmFbOutput.h
        ShmFootmark.h
        SockUtil.h
        UserUtil.h
    )
endif()

target_sources(${component}
    PRIVATE
        ActiveBitTable.cc
        ActivePixelsArray.cc
        Arg.cc
        BinPacketDictionary.cc
        Fb.cc
        FbActivePixels.cc
        FbAov.cc
        FbReferenceType.cc
        Fb_accumulate.cc
        Fb_conv888.cc
        Fb_copy.cc
        Fb_fbd.cc
        Fb_get.cc
        Fb_minusOne.cc
        Fb_ppm.cc
        Fb_setup.cc
        Fb_snapshotDelta.cc
        Fb_untile.cc
        FloatValueTracker.cc
        LatencyLog.cc
        PackActiveTiles.cc
        PackTiles.cc
        PackTilesPassPrecision.cc
        PackTilesTest.cc
        Parser.cc
        PathVisSimGlobalInfo.cc
        PixelBufferSha1Hash.cc
        ProgressiveFrameBufferName.cc
        RenderPrepStats.cc
        RunLenBitTable.cc
        Sha1Util.cc
        VectorPacket.cc
        VectorPacketDictionary.cc
        ${PlatformSpecificSources}
)

set_property(TARGET ${component}
    PROPERTY PUBLIC_HEADER
        ActivePixelsArray.h
        Arg.h
        BinPacketDictionary.h
        Fb.h
        FbActivePixels.h
        FbActivePixelsAov.h
        FbAov.h
        FbReferenceType.h
        FloatValueTracker.h
        LatencyLog.h
        LiteralUtil.h
        PackActiveTiles.h
        PackTiles.h
        PackTilesPassPrecision.h
        PackTilesTest.h
        Parser.h
        PathVisSimGlobalInfo.h
        ProgressiveFrameBufferName.h
        RenderPrepStats.h
        RunLenBitTable.h
        Sha1Util.h
        TlSvr.h
        VectorPacket.h
        VectorPacketDictionary.h
        ${PlatformSpecificPublicHeaders}
)

target_include_directories(${component}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${component}
    PRIVATE
        ${PROJECT_NAME}::render_logging
    PUBLIC
        JsonCpp::JsonCpp
        ${PROJECT_NAME}::common_fb_util
        ${PROJECT_NAME}::common_math
        ${PROJECT_NAME}::common_platform
        ${PROJECT_NAME}::common_rec_time
	${PROJECT_NAME}::render_cache
        ${PROJECT_NAME}::scene_rdl2
        OpenSSL::SSL
        TBB::tbb
)

# If at Dreamworks add a SConscript stub file so others can use this library.
SConscript_Stub(${component})

# Set standard compile/link options
SceneRdl2_cxx_compile_definitions(${component})
SceneRdl2_cxx_compile_features(${component})
SceneRdl2_cxx_compile_options(${component})
SceneRdl2_link_options(${component})

# -------------------------------------
# Install the target and the export set
# -------------------------------------
include(GNUInstallDirs)

# install the target
install(TARGETS ${component}
    COMPONENT ${component}
    EXPORT ${exportGroup}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        NAMELINK_SKIP
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${installIncludeDir}
)

# # install the export set
# install(
#     EXPORT ${exportGroup}
#     NAMESPACE ${PROJECT_NAME}::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
# )
