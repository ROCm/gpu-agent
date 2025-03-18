/*
Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


//------------------------------------------------------------------------------

//
// timestamp related helper functions
//-----------------------------------------------------------------------------

#ifndef __SDK_TIMESTAMP_HPP__
#define __SDK_TIMESTAMP_HPP__

#include <assert.h>
#include <time.h>

// This header file is included from 'C' sources too
#ifdef __cplusplus
#include "include/sdk/base.hpp"
#include "include/sdk/assert.hpp"
namespace sdk {
#else
#define SDK_ASSERT  assert
#include <stdio.h>
#endif  // __cplusplus

// define our typedef on top of struct timespec
typedef struct timespec timespec_t;

#define TIME_NSECS_PER_MSEC                          1000000L
#define TIME_NSECS_PER_USEC                          1000
#define TIME_MSECS_PER_SEC                           1000
#define TIME_NSECS_PER_SEC                           1000000000ULL
#define TIME_USECS_PER_SEC                           1000000
#define TIME_USECS_PER_MSEC                          1000
#define TIME_SECS_PER_MIN                            60
#define TIME_MSECS_PER_MIN                           (60 * TIME_MSECS_PER_SEC)
#define TIME_NSECS_PER_MIN                           (60 * TIME_NSECS_PER_SEC)
#define TIME_HOURS_PER_DAY                           24

#define TIME_STR_SIZE 30

//--------------------------------------------------------------------------
// return true if ts1 < ts2
//--------------------------------------------------------------------------
static inline bool
timestamp_before (timespec_t *ts1, timespec_t *ts2)
{
    if ((ts1->tv_sec < ts2->tv_sec) ||
        ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec < ts2->tv_nsec))) {
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------
// return true if ts1 > ts2
//--------------------------------------------------------------------------
static inline bool
timestamp_later (timespec_t *ts1, timespec_t *ts2)
{
    if ((ts1->tv_sec > ts2->tv_sec) ||
        ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec > ts2->tv_nsec))) {
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------
// return true if ts1 = ts2
//--------------------------------------------------------------------------
static inline bool
timestamp_equal (timespec_t *ts1, timespec_t *ts2)
{
    return ((ts1->tv_sec == ts2->tv_sec) &&
                (ts1->tv_nsec == ts2->tv_nsec));
}

//--------------------------------------------------------------------------
// return true if ts1 <= ts2
//--------------------------------------------------------------------------
static inline bool
timestamp_before_or_equal (timespec_t *ts1, timespec_t *ts2)
{
    if ((ts1->tv_sec <= ts2->tv_sec) ||
        ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec <= ts2->tv_nsec))) {
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------
// return true if ts1 >= ts2
//--------------------------------------------------------------------------
static inline bool
timestamp_equal_or_later (timespec_t *ts1, timespec_t *ts2)
{
    if ((ts1->tv_sec >= ts2->tv_sec) ||
        ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec >= ts2->tv_nsec))) {
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------
// do ts1 += ts2(seconds, nseconds)
//--------------------------------------------------------------------------
static inline void
timestamp_add (timespec_t *ts, long secs, uint64_t nsecs)
{
    ts->tv_nsec += nsecs;
    if ((uint64_t)ts->tv_nsec >= TIME_NSECS_PER_SEC) {
        ts->tv_sec++;
        ts->tv_nsec -= TIME_NSECS_PER_SEC;
    }
    ts->tv_sec += secs;
}

//--------------------------------------------------------------------------
// do ts1 += ts2
//--------------------------------------------------------------------------
static inline void
timestamp_add_timespecs (timespec_t *ts1, timespec_t *ts2)
{
    timestamp_add(ts1, ts2->tv_sec, ts2->tv_nsec);
}

//--------------------------------------------------------------------------
// helper function to compute timestamp diff
//--------------------------------------------------------------------------
static inline void
timestamp_subtract_common (timespec_t *ts1, timespec_t *ts2)
{
    if (timestamp_later(ts1, ts2)) {
        if (ts1->tv_nsec < ts2->tv_nsec) {
            ts1->tv_sec -= 1;
            ts1->tv_nsec += TIME_NSECS_PER_SEC;
        }
        ts1->tv_sec -= ts2->tv_sec;
        ts1->tv_nsec -= ts2->tv_nsec;
    } else {
        ts1->tv_sec = ts1->tv_nsec = 0;
    }
}

//--------------------------------------------------------------------------
// do ts1 -= ts2 if ts1 > ts2 or else return zero timestamp
//--------------------------------------------------------------------------
static inline void
timestamp_subtract (timespec_t *ts1, timespec_t *ts2)
{
    timestamp_subtract_common(ts1, ts2);
}

//---------------------------------------------------------------------------
// return ts = ts1 - ts2 if ts1 > ts2 and else return zero timestamp
//---------------------------------------------------------------------------
static inline timespec_t
timestamp_diff (timespec_t *ts1, timespec_t *ts2)
{
    timespec_t    ts_diff;

    ts_diff = *ts1;
    timestamp_subtract_common(&ts_diff, ts2);
    return ts_diff;
}

//--------------------------------------------------------------------------
// convert time from nanoseconds to timespec format
//--------------------------------------------------------------------------
static inline void
timestamp_from_nsecs (timespec_t *ts, uint64_t nsecs)
{
    assert((ts != NULL) && (nsecs != 0));
    ts->tv_sec = nsecs/TIME_NSECS_PER_SEC;
    ts->tv_nsec = nsecs % TIME_NSECS_PER_SEC;
}

//--------------------------------------------------------------------------
// convert time from timespec format to nanoseconds
//--------------------------------------------------------------------------
static inline void
timestamp_to_nsecs (timespec_t *ts, uint64_t *nsecs)
{
    assert((ts != NULL) && (nsecs != NULL));
    *nsecs = ts->tv_nsec;
    *nsecs += (uint64_t) (ts->tv_sec * TIME_NSECS_PER_SEC);
}

//--------------------------------------------------------------------------
// 1. calculate the duration between ts1 and ts2.
// 2. print the duration in 1.xxxxxxxxx format to buff
//--------------------------------------------------------------------------
static inline size_t
timestamp_diff_to_str (timespec_t *ts1, timespec_t *ts2, char *buff,
                       size_t size)
{
    SDK_ASSERT((ts1 != NULL) && (ts2 != NULL) && (buff != NULL));
    timespec_t diff = timestamp_diff(ts1, ts2);

    return snprintf(buff, size, "%lu.%.9lu", diff.tv_sec, diff.tv_nsec);
}

//--------------------------------------------------------------------------
// print timestamp in Y-m-d H:M:S.e format
//--------------------------------------------------------------------------
static inline size_t
timestamp_str (char *buff, size_t size, timespec_t *ts)
{
    uint32_t   written = 0;

    written = strftime(buff, size, "[%Y-%m-%d %H:%M:%S",
                       localtime(&ts->tv_sec));
    written += snprintf(buff + written, size - written, ".%03lu]",
                        ts->tv_nsec/TIME_NSECS_PER_MSEC);
    return written;
}

#ifdef __cplusplus
//--------------------------------------------------------------------------
// print timestamp in RFC 3339 date-time format with micro sec
//--------------------------------------------------------------------------
static inline std::string
timestamp_rfc3339_datetime_u_str (const struct timeval *ts)
{
    char buff[32];
    uint32_t written = 0;

    // timestamp format 2006-01-02T15:04:05.000000Z
    written = strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S",
                       localtime(&ts->tv_sec));
    written += snprintf(buff + written, sizeof(buff) - written, ".%06luZ",
                        ts->tv_usec);
    return buff;
}

/// \brief    helper API to get time interval between two timespecs in
///           user readable format (min:sec.msec - 00:00.000)
/// \param[in]  start_ts start time in timespec format
/// \param[in]  end_ts   end time in timespec format
/// \return difference between start and end times
static inline std::string
timestamp_interval_to_string (timespec_t *start_ts, timespec_t *end_ts)
{
    uint64_t interval;
    int mins, secs, msecs;
    char buff[TIME_STR_SIZE + 1];
    uint64_t start_nsecs, end_nsecs;

    // convert timestamps to nsecs
    timestamp_to_nsecs(end_ts, &end_nsecs);
    timestamp_to_nsecs(start_ts, &start_nsecs);
    interval = end_nsecs - start_nsecs;
    // convert interval to msecs
    msecs = interval/TIME_NSECS_PER_MSEC;
    // convert msecs to secs
    secs = msecs/TIME_MSECS_PER_SEC;
    // get decimal portion of secs
    msecs = msecs%TIME_MSECS_PER_SEC;
    // convert secs to mins and secs
    mins = secs/TIME_SECS_PER_MIN;
    secs = secs%TIME_SECS_PER_MIN;

    snprintf(buff, TIME_STR_SIZE, "%02d:%02d.%03d", mins, secs, msecs);
    return std::string(buff);
}

}    // namespace sdk

using sdk::timespec_t;
using sdk::timestamp_later;
using sdk::timestamp_add;
#endif  // __cplusplus

#endif    // __SDK_TIMESTAMP_HPP__
