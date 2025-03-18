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
// logger library for SDK
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdbool.h>
#include "logger.h"

#define SDK_LOGGER_MODULE_ID_INVALID    -1
static __thread int t_mod_id = SDK_LOGGER_MODULE_ID_INVALID;
static int g_main_mod_id = SDK_LOGGER_MODULE_ID_INVALID;

#if defined(RTOS)
static __thread logger_trace_cb_t trace_cb_ = NULL;
#else
logger_trace_cb_t trace_cb_ = NULL;
#endif

void
logger_init (logger_trace_cb_t trace_cb)
{
    trace_cb_ = trace_cb;
}

int
null_logger_cb_ (uint32_t mod_id, trace_level_e trace_level,
                 const char *fmt, ...)
{
    return 0;
}

int
stdout_logger_cb_ (uint32_t mod_id, trace_level_e trace_level,
                   const char *fmt, ...)
{
    char logbuf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(logbuf, sizeof(logbuf), fmt, args);
    return printf("%s\n", logbuf);
}

logger_trace_cb_t
logger_trace_cb (void)
{
    return trace_cb_ ? trace_cb_ : stdout_logger_cb_;
}

// register module_id of a module(thread)
// note: register only for those module which
//       run in a separate thread context.
void
logger_register_module_id (int module_id)
{
    t_mod_id = module_id;
    // module-id first module(thread) to register in a process is the
    // main module id, and is the default id used for SDK logging
    if (g_main_mod_id == -1) {
        g_main_mod_id = t_mod_id;
    }
}

int
logger_get_module_id (void)
{
    int module_id;

    if (t_mod_id < 0) {
        if (g_main_mod_id < 0) {
            module_id =  SDK_LOGGER_MODULE_ID_INVALID;
        } else {
            module_id = g_main_mod_id;
        }
    } else {
        module_id = t_mod_id;
    }
    return module_id;
}

bool
logger_is_trace_cb_set (void)
{
    return ((trace_cb_) ? true : false);
}
