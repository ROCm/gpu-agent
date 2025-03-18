
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
/// This file defines base object for all API objects
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_BASE_HPP__
#define __AGA_CORE_API_BASE_HPP__

#include <limits.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/core/obj.h"
#include "nic/gpuagent/core/obj_base.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"

using std::string;

namespace aga {

/// \brief  base class for all api related objects
class api_base : public obj_base {
public:
    /// \brief factory method to instantiate an object
    /// \param[in] api_ctxt API context carrying object related configuration
    static api_base *factory(api_ctxt_t *api_ctxt);

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] obj_id     object identifier
    /// \param[in] api_obj    api object being freed
    /// \return   SDK_RET_OK or error code
    static sdk_ret_t free(obj_id_t obj_id, api_base *api_obj);

    /// \brief add the object to corresponding internal db(s)
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t add_to_db(void) { return SDK_RET_INVALID_OP; }

    /// \brief delete the object from corresponding internal db(s)
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t del_from_db(void) { return SDK_RET_INVALID_OP; }

    /// \brief enqueue the object for delayed destruction
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t delay_delete(void) { return SDK_RET_INVALID_OP; }

    /// \brief find an object based on the object id & key information
    /// \param[in] api_ctxt API context carrying object related information
    static api_base *find_obj(api_ctxt_t *api_ctxt);

    /// \brief find an object based on the object id & key information
    /// \param[in] obj_id    object id
    /// \param[in] key       pointer to the key of the object
    static api_base *find_obj(obj_id_t obj_id, void *key);

    /// \brief return true if object is 'stateless' given an object id
    /// \param[in] obj_id    object id
    static bool stateless(obj_id_t obj_id);

    /// \brief create handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t create_handler(api_params_base *api_params) {
        return SDK_RET_INVALID_OP;
    }

    /// \brief update handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t update_handler(api_params_base *api_params) {
        return SDK_RET_INVALID_OP;
    }

    /// \brief delete handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t delete_handler(api_params_base *api_params) {
        return SDK_RET_INVALID_OP;
    }

    /// \brief returns true if some operation is in progress
    bool in_use(void) const {
        return in_use_;
    }

    /// \brief return stringified key of the object (for debugging)
    virtual string key2str(void) const { return "api_base_key"; }

    /// \brief return key of the object
    virtual const aga_obj_key_t& key(void) const {
        return k_aga_obj_key_invalid;
    }

    /// \brief return stringified contents of the obj (for debugging)
    virtual string tostr(void) const { return "api_base"; }

protected:
    /// \brief constructor
    api_base() {
        in_use_ = FALSE;
    }

    /// \brief destructor
    virtual ~api_base() {};

protected:
    uint8_t in_use_ : 1;
} __PACK__;

}    // namespace aga

using aga::api_base;

#endif    // __AGA_CORE_API_BASE_HPP__
