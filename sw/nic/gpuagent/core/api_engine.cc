
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
/// API message handling by the API "engine"
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_cb.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/core/state_base.hpp"
#include "nic/gpuagent/api/mem.hpp"

namespace aga {

/// \defgroup AGA_API_ENGINE    framework for processing APIs
/// @{

sdk_ret_t
api_engine_init (state_base *state)
{
    return SDK_RET_OK;
}

sdk_ret_t
api_msg_handle_cb (api_msg_t *api_msg, sdk::ipc::ipc_msg_ptr ipc_msg)
{
    api_base *api_obj;
    api_ctxt_t *api_ctxt;
    sdk_ret_t ret = SDK_RET_ERR;
    api_params_base *api_params;
    aga_api_cfg_req_t *req = &api_msg->req;

    // currently we support only one API per API ipc msg
    api_ctxt = req->apis.at(0);
    api_params = api_ctxt->api_params;
    AGA_TRACE_DEBUG("Handling api {} on obj {}, key {}", api_ctxt->api_op,
                    api_ctxt->obj_id, api_ctxt->api_op,
                    api_params->obj_key(api_ctxt->obj_id,
                                        api_ctxt->api_op).str());
    switch (api_ctxt->api_op) {
    case API_OP_CREATE:
        if (api_base::stateless(api_ctxt->obj_id) == false) {
            // check if an object exists with this key
            api_obj = api_base::find_obj(api_ctxt);
            if (unlikely(api_obj != NULL)) {
                // object exists already
                AGA_TRACE_ERR("API object create failed, obj {}, key {} exists "
                              "already", api_ctxt->obj_id,
                              api_params->obj_key(api_ctxt->obj_id,
                                                  api_ctxt->api_op).str());
                return SDK_RET_ENTRY_EXISTS;
            }
        }
        // instantiate new object
        api_obj = api_base::factory(api_ctxt);
        if (unlikely(api_obj == NULL)) {
            AGA_TRACE_ERR("Failed to instantiate new object, obj {}, "
                          "key {}", api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str());
            return SDK_RET_OOM;
        }
        // process the create operation
        ret = g_api_obj_cb[api_ctxt->obj_id].create_cb(api_obj,
                                                       api_ctxt->api_params);
        if (unlikely(ret != SDK_RET_OK)) {
            api_base::free(api_ctxt->obj_id, api_obj);
            AGA_TRACE_ERR("API object create failed, obj id {}, "
                          "key {}, err {}", api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str(), ret());
            return ret;
        }
        if (api_base::stateless(api_ctxt->obj_id)) {
            // if this is stateless object, we can free the object memory
            api_base::free(api_ctxt->obj_id, api_obj);
        } else {
            // add to the object db
            ret = api_obj->add_to_db();
            if (unlikely(ret != SDK_RET_OK)) {
                api_base::free(api_ctxt->obj_id, api_obj);
                AGA_TRACE_ERR("API object db add failed for obj id {}, "
                              "key {}, err {}", api_ctxt->obj_id,
                              api_params->obj_key(api_ctxt->obj_id,
                                                  api_ctxt->api_op).str(),
                              ret());
                return ret;
            }
        }
        break;
    case API_OP_DELETE:
        // check to see if objects exists already
        api_obj = api_base::find_obj(api_ctxt);
        if (unlikely(api_obj == NULL)) {
            AGA_TRACE_ERR("API object delete failed, obj {}, key {} not found",
                          api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str());
            return SDK_RET_ENTRY_NOT_FOUND;
        }
        // process the delete operation
        ret = g_api_obj_cb[api_ctxt->obj_id].delete_cb(api_obj,
                                                       api_ctxt->api_params);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("API object delete failed, obj id {}, "
                          "key {}, err {}", api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str(), ret());
            return ret;
        }
        // remove the object from the db
        ret = api_obj->del_from_db();
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("API object db delete failed for obj id {}, "
                          "key {}, err {}", api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str(), ret());
            return ret;
        }
        // delay delete object
        api_obj->delay_delete();
        break;
    case API_OP_UPDATE:
        // look for the object
        api_obj = api_base::find_obj(api_ctxt);
        if (unlikely(api_obj == NULL)) {
            AGA_TRACE_ERR("API object update failed, obj {}, key {} not found",
                          api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str());
            return SDK_RET_ENTRY_NOT_FOUND;
        }
        // process the update operation
        ret = g_api_obj_cb[api_ctxt->obj_id].update_cb(api_obj,
                                                       api_ctxt->api_params);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("API object update failed, obj id {}, "
                          "key {}, err {}", api_ctxt->obj_id,
                          api_params->obj_key(api_ctxt->obj_id,
                                              api_ctxt->api_op).str(), ret());
            return ret;
        }
        // if this is stateless object, we can free the object memory
        if (api_base::stateless(api_ctxt->obj_id)) {
            api_base::free(api_ctxt->obj_id, api_obj);
        }
        break;
    default:
        AGA_TRACE_ERR("Unknown API operation {} on object {}",
                      api_ctxt->api_op, api_ctxt->obj_id);
        break;
    }
    return ret;
}

/// \@}

}    // namespace aga
