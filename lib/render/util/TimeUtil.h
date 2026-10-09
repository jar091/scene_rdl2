// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

//
//
#pragma once

#include <cstring>     // memset()
#include <ctime>       // std::time(), std::localtime(), std::mktime()
#include <sstream>
#include <string>
#if defined(_WIN32)
// struct timeval comes from <winsock2.h> on Windows; there is no
// gettimeofday, so getCurrentTime() uses <chrono> below.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <chrono>
#else
#include <sys/time.h>  // gettimeofday()
#endif
#include <time.h>      // time_t

namespace scene_rdl2 {
namespace time_util {

inline
void init(struct timeval &tv)
{
    std::memset(&tv, 0x0, sizeof(tv));
}

inline
std::string
timeStr(const struct timeval &tv, bool usec = true)
{
    // tv_sec is `long` (not time_t) inside Windows' timeval; go through a
    // real time_t for localtime() on every platform.
    const time_t sec = static_cast<time_t>(tv.tv_sec);
    struct tm *time_st = localtime(&sec);

    static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    std::ostringstream ostr;
    ostr << time_st->tm_year + 1900 << "/"
         << month[time_st->tm_mon] << "/"
         << time_st->tm_mday << " "
         << wday[time_st->tm_wday] << " "
         << time_st->tm_hour << ":"
         << time_st->tm_min << ":"
         << time_st->tm_sec;
    if (usec) {
        ostr << ":"
             << tv.tv_usec / 1000;
    }
    return ostr.str();
}

inline
std::string
timeStr(const time_t &t)
{
    struct timeval tv;
    tv.tv_sec = static_cast<decltype(tv.tv_sec)>(t);
    tv.tv_usec = 0;
    return timeStr(tv, false);
}

inline
float
utcOffsetHours()
{
    std::time_t currTime = std::time(nullptr);

    const time_t timeLocal = std::mktime(std::localtime(&currTime));

    // Greenwich Mean Time (GMT) and Coordinated Universal Time (UTC) share the same current time in practice
    const time_t timeUTC = std::mktime(std::gmtime(&currTime));

    float diffSec = (float)std::difftime(timeLocal, timeUTC);
    return diffSec / (60.0f * 60.0f);
}

inline
struct timeval
getCurrentTime()
{
    struct timeval tv;
#if defined(_WIN32)
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
    tv.tv_sec = static_cast<decltype(tv.tv_sec)>(sec.count());
    tv.tv_usec = static_cast<decltype(tv.tv_usec)>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - sec).count());
#else
    gettimeofday(&tv, NULL);
#endif
    return tv;
}

inline
std::string
currentTimeStr()
{
    return timeStr(getCurrentTime());
}

} // namespace time_util
} // namespace scene_rdl2

