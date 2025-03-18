


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


#include <google/protobuf/util/json_util.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/gpuagent/core/trace.hpp"

/// \brief this API is invoked only if verbose tracing is enabled AND
///        API tracing is explicitly enabled
/// \param[in] obj          name of the object
/// \param[in] operation    API operation
/// \param[in] msg          pointer to the protobuf API message
void
aga_api_trace_verbose (const char *obj, const char *operation,
                       const google::protobuf::Message *msg)
{
    std::string cfg_str;
    static google::protobuf::util::JsonPrintOptions options;

    if (!((core::trace_level() > trace_level_debug) &&
          (core::api_trace_enabled()))) {
        return;
    }
    // convert to json string
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(*msg, &cfg_str);
    // dump the message contents
    AGA_API_TRACE_NO_META("\"Object\" : \"{}\", "
                          "\"Operation\" : \"{}\", "
                          "\"Config\" : {}", obj, operation, cfg_str.c_str());
}

/// \brief this API is invoked at default trace level settings
/// \param[in] obj          name of the object
/// \param[in] operation    API operation
/// \param[in] key          key of the API object
void
aga_api_trace_terse (const char *obj, const char *operation, const char *key)
{
    if ((core::trace_level() > trace_level_debug) &&
        core::api_trace_enabled()) {
        // aga_api_trace_verbose() would have been invoked in this case,
        // so this is redundant
        return;
    }
    // dump the message contents
    AGA_API_TRACE_NO_META("\"Object\" : \"{}\", \"Operation\" : \"{}\", "
                          "\"Key\" : \"{}\"", obj, operation, key);
}

/// \brief this API is invoked at default trace level settings
/// \param[in] obj          name of the object
/// \param[in] operation    API operation
/// \param[in] proto_msg    protobuf message to be logged
void
aga_api_trace_terse (const char *obj, const char *operation,
                     const google::protobuf::Message *msg)
{
    std::string msg_str;
    static google::protobuf::util::JsonPrintOptions options;

    if ((core::trace_level() > trace_level_debug) &&
        (core::api_trace_enabled())) {
        // aga_api_trace_verbose() would have been invoked in this case,
        // so this is redundant
        return;
    }
    // convert to json string
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(*msg, &msg_str);
    // dump the message contents
    AGA_API_TRACE_NO_META("\"Object\" : \"{}\", \"Operation\" : \"{}\", "
                          "\"Message\" : \"{}\"", obj, operation,
                          msg_str.c_str());
}
