
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
/// GPU object handling
///
//----------------------------------------------------------------------------

#ifndef __AGA_GPU_HPP__
#define __AGA_GPU_HPP__

#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_params.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_task.hpp"

namespace aga {

// attribute update bits for gpu object
#define AGA_GPU_UPD_ADMIN_STATE                 0x1
#define AGA_GPU_UPD_OVERDRIVE_LEVEL             0x2
#define AGA_GPU_UPD_POWER_CAP                   0x4
#define AGA_GPU_UPD_PERF_LEVEL                  0x8
#define AGA_GPU_UPD_CLOCK_FREQ_RANGE            0x10
#define AGA_GPU_UPD_FAN_SPEED                   0x20
#define AGA_GPU_UPD_RAS_SPEC                    0x40

// forward declaration
class gpu_state;

/// \defgroup AGA_GPU_ENTRY - gpu entry object functionality
/// \ingroup AGA
/// @{

/// \brief GPU entry object
class gpu_entry : public api_base {
public:
    /// \brief     factory method to allocate and initialize a gpu entry
    /// \param[in] spec gpu specification
    /// \return    new instance of gpu or NULL, in case of error
    static gpu_entry *factory(aga_gpu_spec_t *spec);

    /// \brief     release all the s/w state associate with the given gpu,
    ///            if any, and free the memory
    /// \param[in] gpu gpu to be freed
    /// \NOTE:     h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this
    static void destroy(gpu_entry *gpu);

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] gpu    gpu to be freed
    /// \return   SDK_RET_OK or error code
    static sdk_ret_t free(gpu_entry *gpu);

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
    sdk_ret_t read(aga_gpu_info_t *info);

    /// \brief return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "gpu-" + std::string(key_.str());
    }

    /// \brief     helper function to get key given gpu entry
    /// \param[in] entry pointer to gpu instance
    /// \return    pointer to the gpu instance's key
    static void *gpu_key_func_get(void *entry) {
        gpu_entry *gpu = (gpu_entry *)entry;
        return (void *)&(gpu->key_);
    }

    /// \brief  return the key of gpu
    /// \return key of the gpu
    virtual const aga_obj_key_t& key(void) const override { return key_; }

    /// \brief  return GPU h/w id (aka. GPU index)
    /// \return GPU index
    uint8_t id(void) const { return id_; }

    /// \brief  set GPU h/w id (aka. GPU index)
    /// \param[in] GPU index
    void set_id(uint8_t id) { id_ = id; }

    /// \brief  return GPU handle
    /// \return GPU handle
    aga_gpu_handle_t handle(void) const { return handle_; }

    /// \brief  set GPU handle
    /// \param[in] GPU handle
    void set_handle(aga_gpu_handle_t handle) {
        handle_ = handle;
    }

    /// \brief return number of GPU watch objects watchnig this GPU
    /// \return     number of GPU watch objects watching this GPU
    uint32_t num_gpu_watch(void) const {
        return num_gpu_watch_;
    }

    /// \brief increment number of GPU watch objects watching this GPU
    void gpu_watch_add(void) {
        num_gpu_watch_++;
    }

    /// \brief decrement number of GPU watch objects watching this GPU
    void gpu_watch_dec(void) {
        num_gpu_watch_--;
    }

    /// \brief  initialize GPU spec
    void init_spec(void) {
        fill_spec_(&spec_);
    }

    /// \brief  return parent GPU uuid
    /// \return parent gpu uuid
    aga_obj_key_t parent_gpu(void) {
        return parent_gpu_;
    }

    /// \brief  set GPU partition
    /// \param[in] GPU partition id
    void set_partition_id(uint32_t partition_id) {
        partition_id_ = partition_id;
    }

    /// \brief  return GPU partition id
    /// \return GPU partition id
    uint32_t partition_id(void) {
        return partition_id_;
    }

    /// \brief  check if GPU is a child
    /// \return true if child GPU, false otherwise
    bool is_child_gpu(void) {
        return parent_gpu_.valid();
    }

    /// \brief check if GPU is a parent GPU
    /// \return true if parent GPU, false otherwise
    bool is_parent_gpu(void) {
        return (child_gpus_.size() > 0);
    }

    /// \brief add child GPU to a parent GPU
    /// \param[in] child_gpu    UUID of child GPU
    void add_child_gpu(aga_obj_key_t *child_gpu) {
        child_gpus_.push_back(*child_gpu);
    }

    /// \brief function to return child GPUs
    /// \return vector of child GPUs
    const std::vector<aga_obj_key_t> child_gpus(void) const {
        return child_gpus_;
    }

    /// \brief function to return compute partition type
    /// \return partition type
    aga_gpu_compute_partition_type_t compute_partition_type(void) {
        return spec_.compute_partition_type;
    }

    /// \brief function to set compute partition type for partitioned GPUs
    /// \return partition type
    void set_compute_partition_type(
             aga_gpu_compute_partition_type_t partition_type) {
        // this function is only used for partitioned GPUs, for child GPUs this
        // is set during init_spec
        if (child_gpus_.size() > 0) {
            spec_.compute_partition_type = partition_type;
        }
    }

    /// \brief     update gpu stats
    /// \param[in] stats gpu stats read by watcher infra
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t update_stats(aga_gpu_watch_fields_t *stats);

    /// \brief      read topology of the GPU
    /// \param[out] info    pointer to the info object
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t read_topology(aga_device_topology_info_t *info);

    /// \brief  return gpu stats
    /// \return gpu stats
    sdk_ret_t fill_gpu_watch_stats(aga_gpu_watch_attrs_t *stats);

private:
    /// \brief constructor
    gpu_entry();

    /// \brief destructor
    ~gpu_entry();

    /// \brief      fill the gpu config spec
    /// \param[out] spec config specification
    void fill_spec_(aga_gpu_spec_t *spec);

    /// \brief      fill the gpu operational status
    /// \param[out] status operational status
    void fill_status_(aga_gpu_status_t *status);

    /// \brief      fill the gpu statistics
    /// \param[out] stats statistics
    void fill_stats_(aga_gpu_stats_t *stats);

private:
    /// uuid of the object
    aga_obj_key_t key_;
    /// uuid of the parent GPU
    aga_obj_key_t parent_gpu_;
    /// partition id, valid only when parent_gpu_ is valid
    uint32_t partition_id_;
    /// vector containing child GPU uuids; used only for parent GPU
    std::vector<aga_obj_key_t> child_gpus_;
    /// GPU id (aka. index)
    uint8_t id_;
    /// GPU handle
    aga_gpu_handle_t handle_;
    /// GPU spec
    aga_gpu_spec_t spec_;
    /// GPU watch stats
    aga_gpu_watch_fields_t stats_;
    /// number of GPU watch objects watching this GPU
    uint32_t num_gpu_watch_;
    /// a friend of gpu entry
    friend class gpu_state;
};

/// \brief    create callback on gpu object
/// \param[in] api_obj    new object instantiated to handle create operation
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_create_cb(api_base *api_obj, api_params_base *api_params);

/// \brief    delete callback on gpu object
/// \param[in] api_obj    object being deleted
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_delete_cb(api_base *api_obj, api_params_base *api_params);

/// \brief    update callback on gpu object
/// \param[in] api_obj    gpu object being updated
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t aga_gpu_update_cb(api_base *api_obj, api_params_base *api_params);

/// \@}

}    // namespace aga

using aga::gpu_entry;

#endif    // __AGA_GPU_HPP_
