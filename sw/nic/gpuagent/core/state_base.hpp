
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
/// base class for the state/db objects
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_STATE_BASE_HPP__
#define __AGA_CORE_STATE_BASE_HPP__

#include "nic/gpuagent/core/obj_base.hpp"
#include "nic/gpuagent/core/api_base.hpp"

namespace aga {

/// \brief     generic walk callback function type for db elements (i.e., api
///            objects)
/// \param[in] obj     object instance
/// \param[in] ctxt    opaque context that the callback can interpret
/// \return    true if walk needs to be stopped or false if walk needs to
///            continue
typedef bool (*state_walk_cb_t)(void *obj, void *ctxt);

/// \brief counters maintained per instance of the state/store class
typedef struct state_counters_s {
    uint32_t insert_ok;     ///< number of successful inserts
    uint32_t insert_err;    ///< number of insert errors
    uint32_t remove_ok;     ///< number of successful deletes
    uint32_t remove_err;    ///< number of delete errors
    uint32_t update_ok;     ///< number of successful updates
    uint32_t update_err;    ///< number of update errors
    uint32_t num_elems;     ///< number of elements/objects currently in store
} state_counters_t;

#define AGA_STATE_CNTR_INSERT_OK_INC()     (counters_.insert_ok++);
#define AGA_STATE_CNTR_REMOVE_OK_INC()     (counters_.remove_ok++);
#define AGA_STATE_CNTR_UPDATE_OK_INC()     (counters_.update_ok++);
#define AGA_STATE_CNTR_INSERT_ERR_INC()    (counters_.insert_err++);
#define AGA_STATE_CNTR_REMOVE_ERR_INC()    (counters_.remove_err++);
#define AGA_STATE_CNTR_UPDATE_ERR_INC()    (counters_.update_err++);
#define AGA_STATE_CNTR_NUM_ELEMS_INC()     (counters_.num_elems++);
#define AGA_STATE_CNTR_NUM_ELEMS_DEC()     (counters_.num_elems--);

/// \brief state base class
class state_base : public obj_base {
public:
    /// \brief constructor
    state_base() {
    };

    /// \brief destructor
    ~state_base(){};

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) {
        return SDK_RET_INVALID_OP;
    }

protected:
    /// state operation counters
    state_counters_t counters_;
};

}    // namespace aga

#endif    // __AGA_CORE_STATE_BASE_HPP__
