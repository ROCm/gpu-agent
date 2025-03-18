
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
/// GPU watch object handling
///
//----------------------------------------------------------------------------

#ifndef __AGA_GPU_WATCH_HPP__
#define __AGA_GPU_WATCH_HPP__

#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_params.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"

namespace aga {

// forward declaration
class gpu_watch_state;

/// \defgroup AGA_GPU_WATCH_ENTRY - gpu watch entry object functionality
/// \ingroup AGA
/// @{

/// \brief GPU watch entry object
class gpu_watch_entry : public api_base {
public:
    /// \brief     factory method to allocate and initialize a gpu entry
    /// \param[in] spec gpu specification
    /// \return    new instance of gpu or NULL, in case of error
    static gpu_watch_entry *factory(aga_gpu_watch_spec_t *spec);

    /// \brief     release all the s/w state associate with the given gpu,
    ///            if any, and free the memory
    /// \param[in] gpu gpu to be freed
    /// \NOTE:     h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this
    static void destroy(gpu_watch_entry *gpu);

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] gpu    gpu to be freed
    /// \return   SDK_RET_OK or error code
    static sdk_ret_t free(gpu_watch_entry *gpu);

    /// \brief  add given gpu to the database
    /// \return SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_to_db(void);

    /// \brief  delete given gpu from the database
    /// \return SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t del_from_db(void);

    /// \brief initiate delay deletion of this object
    virtual sdk_ret_t delay_delete(void) override;

    /// \brief create handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t create_handler(api_params_base *api_params) override;

    /// \brief update handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t update_handler(api_params_base *api_params) override;

    /// \brief delete handler
    /// \param[in] api_params    API parameters capturing key/spec etc
    /// \return   SDK_RET_OK or error code
    virtual sdk_ret_t delete_handler(api_params_base *api_params) override;

    /// \brief          read config
    /// \param[out]     info pointer to the info object
    /// \return         SDK_RET_OK on success, failure status code on error
    sdk_ret_t read(aga_gpu_watch_info_t *info);

    /// \brief return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "gpu-" + std::string(key_.str());
    }

    /// \brief     helper function to get key given gpu entry
    /// \param[in] entry pointer to gpu instance
    /// \return    pointer to the gpu instance's key
    static void *gpu_watch_key_func_get(void *entry) {
        gpu_watch_entry *gpu = (gpu_watch_entry *)entry;
        return (void *)&(gpu->key_);
    }

    /// \brief  return the key of gpu
    /// \return key of the gpu
    virtual const aga_obj_key_t& key(void) const override { return key_; }

    /// \brief    return number of subscribers of the gpu watch
    /// \return   number of subscribers
    uint32_t num_subscriber(void) const {
        return num_subscriber_;
    }

    /// \brief    increment number of subscribers of the gpu watch
    void subscriber_add(void) {
        num_subscriber_++;
    }

    /// \brief    decrement number of subscribers of the gpu watch
    void subscriber_del(void) {
        num_subscriber_--;
    }

private:
    /// \brief constructor
    gpu_watch_entry();

    /// \brief destructor
    ~gpu_watch_entry();

    /// \brief      fill the gpu config spec
    /// \param[out] spec config specification
    void fill_spec_(aga_gpu_watch_spec_t *spec);

    /// \brief      fill the gpu operational status
    /// \param[out] status operational status
    void fill_status_(aga_gpu_watch_status_t *status);

    /// \brief      fill the gpu statistics
    /// \param[out] stats statistics
    void fill_stats_(aga_gpu_watch_stats_t *stats);

private:
    /// uuid of the object
    aga_obj_key_t key_;
    /// GPU watch spec
    aga_gpu_watch_spec_t spec_;
    /// number of subscribers
    uint8_t num_subscriber_;
    /// operational state
    /// a friend of gpu watch entry
    friend class gpu_watch_state;
};

/// \brief    create callback on gpu watch object
/// \param[in] api_obj    new object instantiated to handle create operation
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_watch_create_cb(api_base *api_obj,
                                  api_params_base *api_params);

/// \brief    delete callback on gpu watch object
/// \param[in] api_obj    object being deleted
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_watch_delete_cb(api_base *api_obj,
                                  api_params_base *api_params);

/// \brief    update callback on gpu watch object
/// \param[in] api_obj    gpu object being updated
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_watch_update_cb(api_base *api_obj,
                                  api_params_base *api_params);

/// \@}

}    // namespace aga

using aga::gpu_watch_entry;

#endif    // __AGA_GPU_WATC_HPP_
