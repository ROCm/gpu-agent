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
// logger library macros for SDK code base
//------------------------------------------------------------------------------

#ifndef __SDK_LOGGER_H__
#define __SDK_LOGGER_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include "include/sdk/globals.hpp"
#include "logger_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDK_TRACE(...)              logger_trace_cb()(                                 \
                                        SDK_MOD_ID_SDK,                                \
                                        ##__VA_ARGS__)

#define SDK_TRACE_ERR(fmt, ...)     SDK_TRACE(trace_level_err,                         \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_TRACE_WARN(fmt, ...)    SDK_TRACE(trace_level_warn,                        \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_TRACE_INFO(fmt, ...)    SDK_TRACE(trace_level_info,                        \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_TRACE_DEBUG(fmt, ...)   SDK_TRACE(trace_level_debug,                       \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_TRACE_VERBOSE(fmt, ...) SDK_TRACE(                                         \
                                        trace_level_verbose,                           \
                                        "[%s:%d] " fmt, __FNAME__, __LINE__,           \
                                        ##__VA_ARGS__)

#define SDK_TRACE_PRINT             SDK_TRACE_DEBUG

#define SDK_HMON_TRACE(...)              logger_trace_cb()(                            \
                                             SDK_MOD_ID_HMON,                          \
                                             ##__VA_ARGS__)

#define SDK_HMON_TRACE_ERR(fmt, ...)     SDK_HMON_TRACE(                               \
                                             trace_level_err,                          \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TRACE_WARN(fmt, ...)    SDK_HMON_TRACE(                               \
                                             trace_level_warn,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TRACE_INFO(fmt, ...)    SDK_HMON_TRACE(                               \
                                             trace_level_info,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TRACE_DEBUG(fmt, ...)   SDK_HMON_TRACE(                               \
                                             trace_level_debug,                        \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TRACE_VERBOSE(fmt, ...) SDK_HMON_TRACE(                               \
                                             trace_level_verbose,                      \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_BOOT_TRACE(...)              logger_trace_cb()(                            \
                                             SDK_MOD_ID_BOOT,                          \
                                             ##__VA_ARGS__)

#define SDK_BOOT_TRACE_ERR(fmt, ...)     SDK_BOOT_TRACE(                               \
                                             trace_level_err,                          \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_BOOT_TRACE_WARN(fmt, ...)    SDK_BOOT_TRACE(                               \
                                             trace_level_warn,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_BOOT_TRACE_INFO(fmt, ...)    SDK_BOOT_TRACE(                               \
                                             trace_level_info,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_BOOT_TRACE_DEBUG(fmt, ...)   SDK_BOOT_TRACE(                               \
                                             trace_level_debug,                        \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_BOOT_TRACE_VERBOSE(fmt, ...) SDK_BOOT_TRACE(                               \
                                             trace_level_verbose,                      \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE(...)            logger_trace_cb()(                         \
                                            SDK_MOD_ID_TEMP_HMON,                      \
                                            ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE_ERR(fmt, ...)    SDK_HMON_TEMP_TRACE(                      \
                                             trace_level_err,                          \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE_WARN(fmt, ...)   SDK_HMON_TEMP_TRACE(                      \
                                             trace_level_warn,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE_INFO(fmt, ...)   SDK_HMON_TEMP_TRACE(                      \
                                             trace_level_info,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE_DEBUG(fmt, ...)  SDK_HMON_TEMP_TRACE(                      \
                                             trace_level_debug,                        \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_HMON_TEMP_TRACE_VERBOSE(fmt, ...) SDK_HMON_TEMP_TRACE(                     \
                                              trace_level_verbose,                     \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE(...) logger_trace_cb()(                                \
                                       SDK_MOD_ID_RSC_HMON,                            \
                                       ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE_ERR(fmt, ...) SDK_HMON_RESOURCE_TRACE(                 \
                                              trace_level_err,                         \
                                              "[%s:%d] " fmt, __FNAME__,               \
                                              __LINE__, ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE_WARN(fmt, ...) SDK_HMON_RESOURCE_TRACE(                \
                                               trace_level_warn,                       \
                                               "[%s:%d] " fmt, __FNAME__,              \
                                               __LINE__, ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE_INFO(fmt, ...) SDK_HMON_RESOURCE_TRACE(                \
                                               trace_level_info,                       \
                                               "[%s:%d] " fmt, __FNAME__,              \
                                               __LINE__, ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE_DEBUG(fmt, ...) SDK_HMON_RESOURCE_TRACE(               \
                                                trace_level_debug,                     \
                                                "[%s:%d] " fmt, __FNAME__,             \
                                                __LINE__, ##__VA_ARGS__)

#define SDK_HMON_RESOURCE_TRACE_VERBOSE(fmt, ...) SDK_HMON_RESOURCE_TRACE(             \
                                                  trace_level_verbose,                 \
                                                  "[%s:%d] " fmt, __FNAME__,           \
                                                  __LINE__, ##__VA_ARGS__)

#define SDK_INTR_TRACE(...)              logger_trace_cb()(                            \
                                             SDK_MOD_ID_INTR,                          \
                                             ##__VA_ARGS__)

#define SDK_INTR_TRACE_ERR(fmt, ...)     SDK_INTR_TRACE(                               \
                                             trace_level_err,                          \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_INTR_TRACE_WARN(fmt, ...)    SDK_INTR_TRACE(                               \
                                             trace_level_warn,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_INTR_TRACE_INFO(fmt, ...)    SDK_INTR_TRACE(                               \
                                             trace_level_info,                         \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_INTR_TRACE_DEBUG(fmt, ...)   SDK_INTR_TRACE(                               \
                                             trace_level_debug,                        \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_INTR_TRACE_VERBOSE(fmt, ...) SDK_INTR_TRACE(                               \
                                             trace_level_verbose,                      \
                                             "[%s:%d] " fmt, __FNAME__,                \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE(...) logger_trace_cb()(                                 \
                                        SDK_MOD_ID_INTR_ONETIME,                       \
                                        ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE_ERR(fmt, ...) SDK_INTR_ONETIME_TRACE(                   \
                                                trace_level_err,                       \
                                                "[%s:%d] " fmt, __FNAME__,             \
                                                __LINE__, ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE_WARN(fmt, ...) SDK_INTR_ONETIME_TRACE(                   \
                                                trace_level_warn,                       \
                                                "[%s:%d] " fmt, __FNAME__,              \
                                                __LINE__, ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE_INFO(fmt, ...) SDK_INTR_ONETIME_TRACE(                   \
                                                trace_level_info,                       \
                                                "[%s:%d] " fmt, __FNAME__,              \
                                                __LINE__, ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE_DEBUG(fmt, ...) SDK_INTR_ONETIME_TRACE(                  \
                                                trace_level_debug,                      \
                                                "[%s:%d] " fmt, __FNAME__,              \
                                                __LINE__, ##__VA_ARGS__)

#define SDK_INTR_ONETIME_TRACE_VERBOSE(fmt, ...) SDK_INTR_ONETIME_TRACE(                \
                                                    trace_level_verbose,                \
                                                    "[%s:%d] " fmt, __FNAME__,          \
                                                    __LINE__, ##__VA_ARGS__)

#define SDK_LINK_TRACE(...)              logger_trace_cb()(                             \
                                             SDK_MOD_ID_LINK,                           \
                                             ##__VA_ARGS__)

#define SDK_LINK_TRACE_ERR(fmt, ...)     SDK_LINK_TRACE(                                \
                                             trace_level_err,                           \
                                             "[%s:%d] " fmt, __FNAME__,                 \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_LINK_TRACE_WARN(fmt, ...)    SDK_LINK_TRACE(                                \
                                             trace_level_warn,                          \
                                             "[%s:%d] " fmt, __FNAME__,                 \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_LINK_TRACE_INFO(fmt, ...)    SDK_LINK_TRACE(                                \
                                             trace_level_info,                          \
                                             "[%s:%d] " fmt, __FNAME__,                 \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_LINK_TRACE_DEBUG(fmt, ...)   SDK_LINK_TRACE(                                \
                                             trace_level_debug,                         \
                                             "[%s:%d] " fmt, __FNAME__,                 \
                                             __LINE__, ##__VA_ARGS__)

#define SDK_LINK_TRACE_VERBOSE(fmt, ...) SDK_LINK_TRACE(                                \
                                             trace_level_verbose,                       \
                                             "[%s:%d] " fmt, __FNAME__,                 \
                                             __LINE__, ##__VA_ARGS__)

#if defined(RTOS)
#define PAL_TRACE(...)                   (logger_is_trace_cb_set()) ?                   \
                                             logger_trace_cb()(__VA_ARGS__) :           \
                                             null_logger_cb_(__VA_ARGS__)
#else
#define PAL_TRACE(...)                   logger_trace_cb()(__VA_ARGS__)
#endif

/// \brief    callback function to be used by rtos modules to hook into sdk
///           logger
/// \param[in] log_level    one of the values of log_level_t
/// \param[in] format       format string (followed by variable no. of
///                         parameters)
/// \return    0, if successful, or error code in case of error
static inline int
rtos_sdk_trace_cb (uint32_t log_level, const char *format, ...)
{
    va_list    args;

    va_start(args, format);
    // NOTE:
    // type-casting here requires us to maintain the log/trace level values in
    // sync between sdk and rtos
    SDK_TRACE((trace_level_e)log_level, format, args);
    va_end(args);
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif    // __SDK_LOGGER_H__
