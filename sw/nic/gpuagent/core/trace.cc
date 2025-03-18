
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


//-----------------------------------------------------------------------------
///
/// \file
/// gpuagent trace module
///
//-----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/infra/trace/trace.hpp"

namespace core {

// flag to enable/disable api traces
bool g_api_trace_enabled = false;

// number of generic debug trace files to keep and its size
#define AGA_NUM_TRACE_FILES                     1
#define AGA_TRACE_FILE_SIZE                     (20 << 20)
// number of gpuagent API trace files and their size
#define AGA_API_NUM_TRACE_FILES                1
#define AGA_API_TRACE_FILE_SIZE                (2 << 20)

// logger
utils::log *g_trace_logger;
// api trace logger
utils::log *g_api_trace_logger;

sdk_ret_t
trace_init (void)
{
    const char *logstash_dir;
    const char *persistent_dir;
    const char *non_persistent_dir;
    std::string logfile, err_logfile;
    utils::trace_params_t  params = { 0 };

    non_persistent_dir = std::getenv("NON_PERSISTENT_LOG_DIR");
    persistent_dir = std::getenv("PERSISTENT_LOG_DIR");

    // initialize the gpuagent logger
    logfile = utils::log_file(non_persistent_dir,
                              (non_persistent_dir ? "/gpu-agent.log" :
                                   "/var/run/gpu-agent.log"));
    err_logfile = utils::log_file(persistent_dir, "/var/run/gpu-agent-err.log");

    // agent general log file
    if (logfile.empty() || err_logfile.empty()) {
        return SDK_RET_ERR;
    }

    // initialize trace
    params.name = "gpu-agent";
    params.cores_mask = 0x1;
    params.sync_mode = true;
    params.err_file = err_logfile.c_str();
    params.trace_file = logfile.c_str();
    params.file_size = AGA_TRACE_FILE_SIZE;
    params.num_files = AGA_NUM_TRACE_FILES;
    params.trace_level = trace_level_debug;
    g_trace_logger = utils::trace_init(&params);

    // initialize the API logger
    logstash_dir = std::getenv("LOGSTASH_DIR");
    logfile = utils::log_file(logstash_dir,
                              (logstash_dir ? "/gpu-agent-api.log" :
                                   "/var/run/gpu-agent-api.log"));
    params.name = "gpu-agent-api";
    params.cores_mask = 0x1;
    params.sync_mode = true;
    params.err_file = NULL;
    params.trace_file = logfile.c_str();
    params.file_size = AGA_API_TRACE_FILE_SIZE;
    params.num_files = AGA_API_NUM_TRACE_FILES;
    params.trace_level = trace_level_debug;
    g_api_trace_logger = utils::trace_init(&params);
    g_api_trace_logger->logger()->set_pattern(
        "{\"Timestamp\" : \"%Q %Y-%m-%d %H:%M:%S.%f\", %v}");

    return SDK_RET_OK;
}

// update trace level
void
trace_update (trace_level_e trace_level)
{
    utils::trace_update(trace_level, g_trace_logger);
    utils::trace_update(trace_level, g_api_trace_logger);
    return;
}

// flush logs
void
flush_logs (void)
{
    utils::flush_logs(g_trace_logger);
    utils::flush_logs(g_api_trace_logger);
    return;
}

}    // namespace core
