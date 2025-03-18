
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
/// this file defines trace macros
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_TRACE_HPP__
#define __AGA_CORE_TRACE_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/infra/trace/trace.hpp"

namespace core {

extern bool g_api_trace_enabled;
extern utils::log *g_trace_logger;
extern utils::log *g_api_trace_logger;

static inline bool
api_trace_enabled (void) {
    return g_api_trace_enabled;
}

static inline void
set_api_trace_enabled (bool enable) {
    g_api_trace_enabled = enable;
}

static inline trace_level_e
trace_level (void)
{
    if (g_trace_logger) {
        return g_trace_logger->trace_level();
    }
    return trace_level_none;
}

static inline std::shared_ptr<logger>
trace_logger (void)
{
    if (g_trace_logger) {
        return g_trace_logger->logger();
    }
    return NULL;
}

static inline std::shared_ptr<logger>
api_trace_logger (void)
{
    if (g_api_trace_logger) {
        return g_api_trace_logger->logger();
    }
    return NULL;
}

static inline std::string
err_file_name (void)
{
    if (g_trace_logger) {
        return g_trace_logger->err_file_name();
    }
    return "";
}

static inline std::string
trace_file_name (void)
{
    if (g_trace_logger) {
        return g_trace_logger->trace_file_name();
    }
    return "";
}

sdk_ret_t trace_init(void);
void trace_update(trace_level_e trace_level);
void flush_logs(void);

}    // namespace core

#define AGA_API_TRACE_NO_META(fmt...)                                          \
    if (likely(core::api_trace_logger())) {                                    \
        core::api_trace_logger()->debug(fmt);                                  \
        core::api_trace_logger()->flush();                                     \
    }                                                                          \

#define AGA_TRACE_ERR(fmt, ...)                                                \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_err)) {                            \
        core::trace_logger()->error("[{}:{}] " fmt, __func__, __LINE__,        \
                              ##__VA_ARGS__);                                  \
        core::trace_logger()->flush();                                         \
    }

#define AGA_TRACE_ERR_NO_META(fmt...)                                          \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_err)) {                            \
        core::trace_logger()->error(fmt);                                      \
        core::trace_logger()->flush();                                         \
    }

#define AGA_TRACE_WARN(fmt, ...)                                               \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_warn)) {                           \
        core::trace_logger()->warn("[{}:{}] " fmt, __func__, __LINE__,         \
                              ##__VA_ARGS__);                                  \
        core::trace_logger()->flush();                                         \
    }

#define AGA_TRACE_WARN_NO_META(fmt...)                                         \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_warn)) {                           \
        core::trace_logger()->warn(fmt);                                       \
        core::trace_logger()->flush();                                         \
    }

#define AGA_TRACE_INFO(fmt, ...)                                               \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_info)) {                           \
        core::trace_logger()->info("[{}:{}] " fmt, __func__, __LINE__,         \
                              ##__VA_ARGS__);                                  \
    }

#define AGA_TRACE_INFO_NO_META(fmt...)                                         \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_info)) {                           \
        core::trace_logger()->info(fmt);                                       \
    }

#define AGA_TRACE_DEBUG(fmt, ...)                                              \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_debug)) {                          \
        core::trace_logger()->debug("[{}:{}] " fmt, __func__, __LINE__,        \
                              ##__VA_ARGS__);                                  \
        core::trace_logger()->flush();                                         \
    }

#define AGA_TRACE_DEBUG_NO_META(fmt...)                                        \
    if (likely(core::trace_logger()) &&                                        \
        (core::trace_level() >= trace_level_debug)) {                          \
        core::trace_logger()->debug(fmt);                                      \
    }

#define AGA_TRACE_VERBOSE(fmt, ...)                                            \
    if (likely(core::trace_logger())) {                                        \
        core::trace_logger()->trace("[{}:{}] " fmt, __func__, __LINE__,        \
                              ##__VA_ARGS__);                                  \
    }

#define AGA_TRACE_VERBOSE_NO_META(fmt...)                                      \
    if (likely(core::trace_logger())) {                                        \
        core::trace_logger()->trace(fmt);                                      \
    }                                                                          \

#define AGA_MOD_TRACE_ERR_NO_META(mod_id, fmt...)        AGA_TRACE_ERR_NO_META(fmt)
#define AGA_MOD_TRACE_WARN_NO_META(mod_id, fmt...)       AGA_TRACE_WARN_NO_META(fmt)
#define AGA_MOD_TRACE_INFO_NO_META(mod_id, fmt...)       AGA_TRACE_INFO_NO_META(fmt)
#define AGA_MOD_TRACE_DEBUG_NO_META(mod_id, fmt...)      AGA_TRACE_DEBUG_NO_META(fmt)
#define AGA_MOD_TRACE_VERBOSE_NO_META(mod_id, fmt...)    AGA_TRACE_VERBOSE_NO_META(fmt)

#endif    // __AGA_CORE_TRACE_HPP__
