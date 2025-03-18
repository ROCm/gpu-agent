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
//----------------------------------------------------------------------------
///
/// \file
/// base header file for logger module
///
//----------------------------------------------------------------------------

#ifndef __SDK_LOGGER_BASE_H__
#define __SDK_LOGGER_BASE_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __STDC_FORMAT_MACROS

typedef enum {
    trace_level_none    = 0,        // traces disabled completely
    trace_level_err     = 1,
    trace_level_warn    = 2,
    trace_level_info    = 3,
    trace_level_debug   = 4,
    trace_level_verbose = 5,
} trace_level_e;

typedef int (*logger_trace_cb_t)(uint32_t mod_id, trace_level_e trace_level,
                                 const char *format, ...)
                                 __attribute__((format (printf, 3, 4)));

void logger_register_module_id(int module_id);
int logger_get_module_id(void);
void logger_init(logger_trace_cb_t trace_cb);
int null_logger_cb_(uint32_t mod_id, trace_level_e trace_level,
                    const char *fmt, ...);
int stdout_logger_cb_(uint32_t mod_id, trace_level_e trace_level,
                      const char *fmt, ...);
logger_trace_cb_t logger_trace_cb(void);
bool logger_is_trace_cb_set(void);

#ifdef __cplusplus
}
#endif
#endif    // __SDK_LOGGER_BASE_H__
