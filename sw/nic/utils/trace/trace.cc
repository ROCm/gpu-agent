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


#include <chrono>
#include "nic/infra/trace/trace.hpp"

namespace utils {

// logger class static constants
const size_t log::k_async_qsize_ = 128 * 1024;    // must be power of 2
const spdlog::async_overflow_policy log::k_async_overflow_policy_ = spdlog::async_overflow_policy::discard_log_msg;
const std::chrono::milliseconds log::k_flush_intvl_ms_ = std::chrono::milliseconds(50);
uint64_t g_logger_cpu_mask = 0;

// logger class methods
spdlog::level::level_enum
log::trace_level_to_spdlog_level(trace_level_e level) {
    switch (level) {
    case trace_level_none:
        return spdlog::level::off;

    case trace_level_err:
        return spdlog::level::err;

    case trace_level_warn:
        return spdlog::level::warn;

    case trace_level_info:
        return spdlog::level::info;

    case trace_level_debug:
        return spdlog::level::debug;

    case trace_level_verbose:
        return spdlog::level::trace;

    default:
        return spdlog::level::off;
    }
}

spdlog::level::level_enum
log::syslog_level_to_spdlog_level(sdk::types::syslog_level_e level) {
    switch (level) {
    case sdk::types::log_none:
        return spdlog::level::off;

    case sdk::types::log_alert:
    case sdk::types::log_emerg:
    case sdk::types::log_crit:
        return spdlog::level::critical;

    case sdk::types::log_err:
        return spdlog::level::err;

    case sdk::types::log_warn:
        return spdlog::level::warn;

    case sdk::types::log_notice:
    case sdk::types::log_info:
        return spdlog::level::info;

    case sdk::types::log_debug:
        return spdlog::level::debug;

    default:
        return spdlog::level::off;
    }
}

void
log::set_cpu_affinity(void) {
    cpu_set_t   cpus;
    uint64_t    cpu_mask = g_logger_cpu_mask;

    CPU_ZERO(&cpus);
    while (cpu_mask != 0) {
        CPU_SET(ffsl(cpu_mask) - 1, &cpus);
        cpu_mask = cpu_mask & (cpu_mask - 1);
    }
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpus);
}

bool
log::init(const char *name, uint64_t cpu_mask, log_mode_e log_mode,
          bool syslogger, const char *persistent_file_name,
          const char *non_persistent_file_name,
          size_t file_size, size_t max_files,
          trace_level_e persistent_trace_level,
          trace_level_e non_persistent_trace_level,
          syslog_level_e syslog_level,
          bool truncate) {
    std::function<void()> worker_thread_cb = set_cpu_affinity;

    // first time when *any* logger is created, save the cpu mask and use it
    // for all other logger instances as well
    if (!g_logger_cpu_mask) {
        if (!cpu_mask) {
            g_logger_cpu_mask = 0x1;
        } else {
            g_logger_cpu_mask = cpu_mask;
        }
        g_logger_cpu_mask = cpu_mask;
    }
    syslogger_ = syslogger;
    trace_level_ = non_persistent_trace_level;
    log_level_ = syslog_level;
    if (persistent_file_name) {
        err_file_name_ = std::string(persistent_file_name);
    } else {
        err_file_name_ = std::string("");
    }
    if (non_persistent_file_name) {
        trace_file_name_ = std::string(non_persistent_file_name);
    } else {
        trace_file_name_ = std::string("");
    }
    if (log_mode == sdk::types::log_mode_async) {
        spdlog::set_async_mode(k_async_qsize_, k_async_overflow_policy_,
                               worker_thread_cb, k_flush_intvl_ms_, NULL);
    }
    if (syslogger) {
        logger_ = spdlog::syslog_logger(name, name, LOG_PID);
    } else {

        auto dist_sink = std::make_shared<spdlog::sinks::dist_sink_mt>();

        if (persistent_file_name &&
            strcmp(persistent_file_name, "") != 0) {
            auto sink_persist = std::make_shared <spdlog::sinks::rotating_file_sink_mt>
                (persistent_file_name, file_size, max_files);
            sink_persist->set_level(
                trace_level_to_spdlog_level(persistent_trace_level));
            dist_sink->add_sink(sink_persist);
        }

        if (non_persistent_file_name &&
            strcmp(non_persistent_file_name, "") != 0) {
            auto sink_non_persist = std::make_shared<spdlog::sinks::rotating_file_sink_mt>
                (non_persistent_file_name, file_size, max_files);
            sink_non_persist->set_level(
                trace_level_to_spdlog_level(trace_level_verbose));
            dist_sink->add_sink(sink_non_persist);
        }
        logger_ = std::make_shared<spdlog::logger>(name, dist_sink);
    }
    if (logger_) {
        logger_->set_pattern("%L [%Q %Y-%m-%d %H:%M:%S.%f] (%t) %v");
        if (syslogger) {
            logger_->set_level(syslog_level_to_spdlog_level(syslog_level));
        } else {
            logger_->set_level(trace_level_to_spdlog_level(trace_level_));
        }

        // trigger flush if the log severity is error or higher
        logger_->flush_on(spdlog::level::err);
        return true;
    }
    return false;
}

log *
log::factory(const char *name, uint64_t cpu_mask, log_mode_e log_mode,
             bool syslogger, const char *persistent_trace_file_name,
             const char *non_persistent_file_name,
             size_t file_size, size_t max_files,
             trace_level_e persistent_trace_level,
             trace_level_e non_persistent_trace_level,
             syslog_level_e syslog_level,
             bool truncate) {
    void    *mem;
    log     *new_logger;

    if (!name || (!persistent_trace_file_name && !non_persistent_file_name)) {
        return NULL;
    }

    mem = calloc(1, sizeof(log));
    if (!mem) {
        return NULL;
    }

    new_logger = new (mem) log();
    if (new_logger->init(name, cpu_mask, log_mode, syslogger,
                         persistent_trace_file_name, non_persistent_file_name,
                         file_size, max_files, persistent_trace_level,
                         non_persistent_trace_level, syslog_level,
                         truncate) == false) {
        new_logger->~log();
        free(new_logger);
        return NULL;
    }
    return new_logger;
}

log::~log() {
    //spdlog::drop(name_);
}

void
log::destroy(log *logger_obj) {
    if (!logger_obj) {
        return;
    }
    logger_obj->~log();
    free(logger_obj);
}

void
log::set_trace_level(trace_level_e level) {
    if (!syslogger_) {
        logger_->set_level(trace_level_to_spdlog_level(level));
        trace_level_ = level;
    }
}

void
log::set_syslog_level(syslog_level_e level) {
    if (syslogger_) {
        logger_->set_level(syslog_level_to_spdlog_level(level));
        log_level_ = level;
    }
}

void
log::flush(void) {
    logger_->flush();
}

std::shared_ptr<spdlog::logger>
log::logger(void) {
    return logger_;
}

//------------------------------------------------------------------------------
// initialize trace lib
//------------------------------------------------------------------------------
log *
trace_init (trace_params_t *params)
{
    if ((params == NULL) || (params->name == NULL) ||
        (params->trace_file  == NULL)) {
        return NULL;
    }
    return
        log::factory(params->name, params->cores_mask,
            params->sync_mode ? sdk::types::log_mode_sync :
                                sdk::types::log_mode_async,
            false, params->err_file, params->trace_file,
            params->file_size, params->num_files, trace_level_err,
            params->trace_level, sdk::types::log_none);
}

//------------------------------------------------------------------------------
// cleanup trace lib
//------------------------------------------------------------------------------
void
trace_cleanup (log *trace_logger)
{
    if (trace_logger) {
        utils::log::destroy(trace_logger);
    }
    trace_logger = NULL;
    return;
}

//------------------------------------------------------------------------------
// change trace level
//------------------------------------------------------------------------------
void
trace_update (trace_level_e trace_level, log *trace_logger)
{
    trace_logger->set_trace_level(trace_level);
    return;
}

//------------------------------------------------------------------------------
// flush logs
//------------------------------------------------------------------------------
void
flush_logs (log *trace_logger)
{
    if (trace_logger) {
        trace_logger->flush();
    }
    return;
}
}    // utils
