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
/// smi layer API definitions
///
//----------------------------------------------------------------------------

#include "nic/third-party/rocm/rocm_smi_lib/include/rocm_smi/rocm_smi.h"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/rocmsmi/smi_utils.hpp"

// TODO:
// not using aga_ here for proper naming !!!

namespace aga {

/// \brief    fill clock frequency ranges of the given GPU
/// \param[in] gpu_id    GPU id
/// \param[out] spec     spec to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_clock_frequency_spec_ (uint32_t gpu_id, aga_gpu_spec_t *spec)
{
    uint32_t clk_cnt = 0;
    rsmi_status_t rsmi_ret;
    rsmi_od_volt_freq_data_t vc_data = {};
    aga_gpu_clock_freq_range_t *clock_spec;

    // this api is called twice, once for spec (clock spec) and
    // once for status (voltage-curve-points)
    rsmi_ret = rsmi_dev_od_volt_info_get(gpu_id, &vc_data);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get clock frequency spec for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        // vc_data has mClock and sClock current and possible ranges
        // fill sClock spec
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(RSMI_CLK_TYPE_SYS);
        // min and max frequencies are per clock type
        clock_spec->lo = vc_data.curr_sclk_range.lower_bound/1000000;
        clock_spec->hi = vc_data.curr_sclk_range.upper_bound/1000000;
        clk_cnt++;

        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(RSMI_CLK_TYPE_MEM);
        // min and max frequencies are per clock type
        clock_spec->lo = vc_data.curr_mclk_range.lower_bound/1000000;
        clock_spec->hi = vc_data.curr_mclk_range.upper_bound/1000000;
        clk_cnt++;
        spec->num_clock_freqs = clk_cnt;
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_spec (uint32_t gpu_id, aga_gpu_spec_t *spec)
{
    uint32_t value_32;
    uint64_t value_64;
    rsmi_status_t rsmi_ret;
    rsmi_dev_perf_level_t perf_level;

    // fill the overdrive level
    rsmi_ret = rsmi_dev_overdrive_level_get(gpu_id, &value_32);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get clock overdrive for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        spec->overdrive_level = value_32;
    }
    // fill the power overdrive level
    rsmi_ret = rsmi_dev_power_cap_get(gpu_id, 0, &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power overdrive for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        spec->gpu_power_cap = value_64/1000000;
    }
    // fill the perf level
    rsmi_ret = rsmi_dev_perf_level_get(gpu_id, &perf_level);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get performance level gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        spec->perf_level = smi_to_aga_gpu_perf_level(perf_level);
    }
    // fill gpu and memory clock frequencies
    smi_fill_gpu_clock_frequency_spec_(gpu_id, spec);
    return SDK_RET_OK;
}

/// \brief     function to get name for rsmi firmware block enum
/// \param[in] block    rsmi firmware block enum
/// \return    firmware block name
static inline const char *
gpu_fw_block_name_str_ (rsmi_fw_block_t block)
{
    switch (block) {
    case RSMI_FW_BLOCK_ASD:
        return "ASD";
    case RSMI_FW_BLOCK_CE:
        return"CE";
    case RSMI_FW_BLOCK_DMCU:
        return "DMCU";
    case RSMI_FW_BLOCK_MC:
        return "MC";
    case RSMI_FW_BLOCK_ME:
        return "ME";
    case RSMI_FW_BLOCK_MEC:
        return "MEC";
    case RSMI_FW_BLOCK_MEC2:
        return "MEC2";
    case RSMI_FW_BLOCK_MES:
        return "MES";
    case RSMI_FW_BLOCK_MES_KIQ:
        return "MES_KIQ";
    case RSMI_FW_BLOCK_PFP:
        return "PFP";
    case RSMI_FW_BLOCK_RLC:
        return "RLC";
    case RSMI_FW_BLOCK_RLC_SRLC:
        return "RLC_SRLC";
    case RSMI_FW_BLOCK_RLC_SRLG:
        return "RLC_SRLG";
    case RSMI_FW_BLOCK_RLC_SRLS:
        return "RLC_SRLS";
    case RSMI_FW_BLOCK_SDMA:
        return "SDMA";
    case RSMI_FW_BLOCK_SDMA2:
        return "SDMA2";
    case RSMI_FW_BLOCK_SMC:
        return "SMC";
    case RSMI_FW_BLOCK_SOS:
        return "SOS";
    case RSMI_FW_BLOCK_TA_RAS:
        return "TA_RAS";
    case RSMI_FW_BLOCK_TA_XGMI:
        return "TA_XGMI";
    case RSMI_FW_BLOCK_UVD:
        return "UVD";
    case RSMI_FW_BLOCK_VCE:
        return "VCE";
    case RSMI_FW_BLOCK_VCN:
        return "VCN";
    };
    return "Unknown firmware component";
}

/// \brief      function to format firmware version
/// \param[out] fw_version    firmware component/version after formatting
/// \param[in]  block         firmware component enum
/// \param[in]  version       firmware version
/// \return     none
static void
fill_gpu_fw_version_ (aga_gpu_fw_version_t *fw_version, rsmi_fw_block_t block,
                      uint64_t version)
{
    char buf[AGA_MAX_STR_LEN + 1];

    strncpy(fw_version->firmware, gpu_fw_block_name_str_(block),
            AGA_MAX_STR_LEN);
    if ((block == RSMI_FW_BLOCK_VCN) || (block == RSMI_FW_BLOCK_UVD) ||
        (block == RSMI_FW_BLOCK_VCE) || (block == RSMI_FW_BLOCK_SOS) ||
        (block == RSMI_FW_BLOCK_ASD) || (block == RSMI_FW_BLOCK_MES) ||
        (block == RSMI_FW_BLOCK_MES_KIQ)) {
        // 'VCN', 'VCE', 'UVD', 'SOS', 'ASD', 'MES', 'MES KIQ' fw versions
        // needs to hexadecimal
        snprintf(buf, AGA_MAX_STR_LEN, "0x%08" PRIx64, version);
        strncpy(fw_version->version, buf, AGA_MAX_STR_LEN);
    } else if ((block == RSMI_FW_BLOCK_TA_XGMI) ||
               (block == RSMI_FW_BLOCK_TA_RAS) ||
               (block == RSMI_FW_BLOCK_SMC)) {
        // TA XGMI, TA RAS, and SMC firmware's hex value looks like 0x12345678
        // however, they are parsed as: int(0x12).int(0x34).int(0x56).int(0x78)
        // which results in the following: 12.34.56.78
        unsigned char tmp[8];
        for(auto i = 0; i < 8; i++) {
            tmp[i] = version >> ((7-i)*8);
        }
        snprintf(buf, AGA_MAX_STR_LEN, "%02u.%02u.%02u.%02u",
                 tmp[4], tmp[5], tmp[6], tmp[7]);
        strncpy(fw_version->version, buf, AGA_MAX_STR_LEN);
    } else {
        strncpy(fw_version->version, std::to_string(version).c_str(),
                AGA_MAX_STR_LEN);
    }
}

/// \brief      get SKU from VBIOS version
/// \param[in]  vbios_ver    VBIOS version string
/// \param[out] sku          SKU string dervied from vbios version
/// \return     none
static void
gpu_get_sku_from_vbios_ (char *vbios_ver, char *sku)
{
    char *buf;
    char *token;

    // middle portion in the VBIOS version is SKU XXX-<CARD_SKU>-XXX
    // get first token
    token = strtok_r(vbios_ver, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}",
                      vbios_ver);
        return;
    }
    // second token is the SKU
    token = strtok_r(NULL, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}",
                      vbios_ver);
        return;
    }
    strncpy(sku, token, AGA_MAX_STR_LEN);
}

/// \brief    fill list of pids using the given GPU
/// \param[in] gpu_id    GPU id
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_kfd_pid_status_ (uint32_t gpu_id, aga_gpu_status_t *status)
{
    rsmi_status_t rsmi_ret;
    uint32_t gpu_list[AGA_MAX_GPU];
    rsmi_process_info_t *pid_info;
    uint32_t value_32, num_pid = 0, num_gpus = AGA_MAX_GPU;

    // kernel fusion driver pids
    rsmi_ret = rsmi_compute_process_info_get(NULL, &value_32);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get KFD pid count, err gpu {}, {}",
                      gpu_id, rsmi_ret);
    } else {
        // if pid count is non zero, get the pid info
        if (value_32) {
            pid_info =
                (rsmi_process_info_t *)malloc(sizeof(rsmi_process_info_t) *
                                              value_32);
            if (pid_info == NULL) {
                AGA_TRACE_ERR("Failed to allocate KFD pid buffer, gpu {}");
                return SDK_RET_OOM;
            }
            rsmi_ret = rsmi_compute_process_info_get(pid_info, &value_32);
            if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to get KFD pid info, gpu {}, err {}",
                              gpu_id, rsmi_ret);
                return rsmi_ret_to_sdk_ret(rsmi_ret);
            }
            // loop thru pids, get the list of GPUs using each pid and
            // update per GPU kfd process list
            for (uint32_t i = 0; i < value_32; i++) {
                num_gpus = AGA_MAX_GPU;
                rsmi_ret =
                    rsmi_compute_process_gpus_get(pid_info[i].process_id,
                                                  gpu_list, &num_gpus);
                if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                    AGA_TRACE_ERR("Failed to get gpu list of pid {} for "
                                  "gpu {}, err {}", pid_info[i].process_id,
                                  gpu_id, rsmi_ret);
                    continue;
                }
                for (uint32_t j = 0; j < num_gpus; j++) {
                    if (gpu_list[i] == gpu_id) {
                        if (num_pid == (AGA_GPU_MAX_KFD_PID - 1)) {
                            AGA_TRACE_DEBUG("Reached max KFD processes {} "
                                            "using the GPU {}, pid {} is "
                                            "ignored", AGA_GPU_MAX_KFD_PID,
                                            gpu_id, pid_info[i].process_id);
                            break;
                        }
                        status->kfd_process_id[num_pid++] =
                            pid_info[i].process_id;
                        break;
                    }
                }
            }
            status->num_kfd_process_id = num_pid;
            // free pid_info memory
            free (pid_info);
        }
    }
    return SDK_RET_OK;
}

/// \brief    fill supported and current frequencies of system clocks
/// \param[in] gpu_id    GPU id
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_clock_status_ (uint32_t gpu_id, aga_gpu_status_t *status)
{
    uint32_t clk_cnt = 0;
    rsmi_status_t rsmi_ret;
    rsmi_frequencies_t freq;

    // get the fClock frequency information
    rsmi_ret = rsmi_dev_gpu_clk_freq_get(gpu_id, RSMI_CLK_TYPE_DF, &freq);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get fClock frequency information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        // fill the current fClock status
        auto clk_st = &status->clock_status[clk_cnt++];
        clk_st->clock_type = AGA_GPU_CLOCK_TYPE_FABRIC;
        clk_st->frequency = freq.frequency[freq.current]/1000000;
    }
    // get the mClock frequency information
    rsmi_ret = rsmi_dev_gpu_clk_freq_get(gpu_id, RSMI_CLK_TYPE_MEM, &freq);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get mClock frequency information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        // fill the current mClock status
        auto clk_st = &status->clock_status[clk_cnt++];
        clk_st->clock_type = AGA_GPU_CLOCK_TYPE_MEMORY;
        clk_st->frequency = freq.frequency[freq.current]/1000000;
    }
    // get the sClock frequency information
    rsmi_ret = rsmi_dev_gpu_clk_freq_get(gpu_id, RSMI_CLK_TYPE_SYS, &freq);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get sClock frequency information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        // fill the current sClock status
        auto clk_st = &status->clock_status[clk_cnt++];
        clk_st->clock_type = AGA_GPU_CLOCK_TYPE_SYSTEM;
        clk_st->frequency = freq.frequency[freq.current]/1000000;
    }
    // get the SoCClock frequency information
    rsmi_ret = rsmi_dev_gpu_clk_freq_get(gpu_id, RSMI_CLK_TYPE_SOC, &freq);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get SoCClock frequency information for "
                      "gpu {}, err {}", gpu_id, rsmi_ret);
    } else {
        // fill the current sClock status
        auto clk_st = &status->clock_status[clk_cnt++];
        clk_st->clock_type = AGA_GPU_CLOCK_TYPE_SOC;
        clk_st->frequency = freq.frequency[freq.current]/1000000;
    }
    return SDK_RET_OK;
}

/// \brief    fill PCIe status
/// \param[in] gpu_id     GPU id
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_pcie_status_ (uint32_t gpu_id,
                       aga_gpu_status_t *status)
{
    rsmi_status_t rsmi_ret;
    uint64_t value_64, sent, rcvd, max_size;
    aga_gpu_pcie_status_t *pcie_status = &status->pcie_status;

    // fill the PCIe bandwidth
    rsmi_ret = rsmi_dev_pci_throughput_get(gpu_id, &sent, &rcvd,
                                           &max_size);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe bandwidth for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
       pcie_status->bandwidth = ((rcvd + sent) * max_size) / 1024 / 1024;
    }
    // fill the PCIe bus id
    rsmi_ret = rsmi_dev_pci_id_get(gpu_id, &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe bus id for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        // convert PCIe bus to XXXX.XX.XX.X format
        snprintf(pcie_status->pcie_bus_id, AGA_MAX_STR_LEN, "%04X:%02X:%02X.%X",
                ((uint32_t)((value_64 >> 32) & 0xffffffff)),
                ((uint32_t)((value_64 >> 8) & 0xff)),
                ((uint32_t)((value_64 >> 3) & 0x1f)),
                ((uint32_t)(value_64 & 0x7)));
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_status (aga_gpu_handle_t gpu_handle, uint32_t gpu_id,
                     aga_gpu_status_t *status)
{
    uint32_t i;
    uint64_t ver, value_64;
    rsmi_status_t rsmi_ret;
    rsmi_fw_block_t fw_block;
    rsmi_xgmi_status_t xgmi_st;
    char buf[AGA_MAX_STR_LEN + 1];
    rsmi_od_volt_freq_data_t vc_data = {};

    status->index = gpu_id;
    // fill the GPU serial number
    rsmi_ret = rsmi_dev_serial_number_get(gpu_id, status->serial_num,
                                          AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
       AGA_TRACE_ERR("Failed to get serial number for gpu {}, err {}",
                     gpu_id, rsmi_ret);
    }
    // fill the GPU card series
    rsmi_ret = rsmi_dev_name_get(gpu_id, status->card_series,
                                 AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
       AGA_TRACE_ERR("Failed to get card series for gpu {}, err {}",
                     gpu_id, rsmi_ret);
    }
    // fill the GPU card model
    rsmi_ret = rsmi_dev_subsystem_name_get(gpu_id, status->card_model,
                                           AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
       AGA_TRACE_ERR("Failed to get card model for gpu {}, err {}",
                     gpu_id, rsmi_ret);
    }
    // fill the GPU vendor information
    rsmi_ret = rsmi_dev_vendor_name_get(gpu_id, status->card_vendor,
                                        AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get vendor for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    }
    // fill the driver version
    rsmi_ret = rsmi_version_str_get(RSMI_SW_COMP_DRIVER, status->driver_version,
                                    AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system driver version, gpu {}, err {}",
                      gpu_id, rsmi_ret);
    }
    // fill the vbios version
    rsmi_ret = rsmi_dev_vbios_version_get(gpu_id, buf, AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get vbios version for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        strncpy(status->vbios_part_number, buf, AGA_MAX_STR_LEN);
        // sku should be retrieved from vbios version
        // TODO: clean the parameter order here !!
        gpu_get_sku_from_vbios_(buf, status->card_sku);
    }
    // fill the firmwre version
    for (i = 0, fw_block = RSMI_FW_BLOCK_FIRST; fw_block <= RSMI_FW_BLOCK_LAST;
         fw_block = (rsmi_fw_block_t)(((uint32_t)fw_block) + 1)) {
        rsmi_ret = rsmi_dev_firmware_version_get(gpu_id, fw_block, &ver);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get firmware version for gpu {} component "
                          "{}, err {}", gpu_id, fw_block, rsmi_ret);
        } else {
            fill_gpu_fw_version_(&status->fw_version[i++], fw_block, ver);
            status->num_fw_versions = i;
        }
    }
    // fill the memory vendor
    rsmi_ret = rsmi_dev_vram_vendor_get(gpu_id, status->memory_vendor,
                                        AGA_MAX_STR_LEN);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory vendor for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    }
    // fill the max supported clock frequencies and current frequency
    smi_fill_clock_status_(gpu_id, status);
    // fill the PCIe status
    smi_fill_pcie_status_(gpu_id, status);
    // fill the xgmi error count
    rsmi_ret = rsmi_dev_xgmi_error_status(gpu_id, &xgmi_st);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get xgmi error status for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        status->xgmi_error_status = smi_to_aga_gpu_xgmi_error(xgmi_st);
    }
    // fill the voltage curve points
    rsmi_ret = rsmi_dev_od_volt_info_get(gpu_id, &vc_data);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get voltage curve points for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        for (uint32_t i = 0;
             (i < AGA_GPU_MAX_VOLTAGE_CURVE_POINT) &&
             (i < RSMI_NUM_VOLTAGE_CURVE_POINTS); i++) {
            status->voltage_curve_point[i].point = i;
            status->voltage_curve_point[i].frequency =
                vc_data.curve.vc_points[i].frequency/1000000;
            status->voltage_curve_point[i].voltage =
                vc_data.curve.vc_points[i].voltage;
        }
    }
    smi_fill_gpu_kfd_pid_status_(gpu_id, status);
    // fill total memory
    rsmi_ret = rsmi_dev_memory_total_get(gpu_id, RSMI_MEM_TYPE_VRAM,
                                         &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM total memory gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        status->vram_status.size = value_64/1024/1024;
    }
    // TODO: get admin_state
    // TODO: get RAS spec
    // TODO: oper status
    // TODO: RAS status
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_stats (uint32_t gpu_id, aga_gpu_stats_t *stats)
{
	int64_t voltage;
    int64_t temperature;
    rsmi_status_t rsmi_ret;
    float counter_resolution;
    rsmi_activity_metric_counter_t activity;
    uint64_t power, value_64;

    // fill the avg package power
    // TODO: what is sensor_ind (second arg) here?
    rsmi_ret = rsmi_dev_power_ave_get(gpu_id, 0, &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get avg graphics power for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->avg_package_power = value_64/1000000;
    }
    // fill the GPU usage
    rsmi_ret = rsmi_dev_activity_metric_get(gpu_id, RSMI_ACTIVITY_GFX,
                                            &activity);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get gfx activity information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        if (activity.average_gfx_activity != 0xffff) {
            stats->usage.gfx_activity = activity.average_gfx_activity;
        }
    }
    rsmi_ret = rsmi_dev_activity_metric_get(gpu_id, RSMI_ACTIVITY_UMC,
                                            &activity);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get umc activity information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        if (activity.average_umc_activity != 0xffff) {
            stats->usage.umc_activity = activity.average_umc_activity;
        }
    }
    rsmi_ret = rsmi_dev_activity_metric_get(gpu_id, RSMI_ACTIVITY_MM,
                                            &activity);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get mm activity information for gpu {}, "
                      "err {}", gpu_id, rsmi_ret);
    } else {
        if (activity.average_mm_activity != 0xffff) {
            stats->usage.mm_activity = activity.average_mm_activity;
        }
    }
    // fill the voltage
    rsmi_ret = rsmi_dev_volt_metric_get(gpu_id, RSMI_VOLT_TYPE_FIRST,
                                        RSMI_VOLT_CURRENT, &voltage);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe bandwidth for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
       stats->voltage.voltage = voltage/1000;
    }
    // fill the PCIe replay count
    rsmi_ret = rsmi_dev_pci_replay_counter_get(gpu_id, &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe replay counter for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
       stats->pcie_stats.replay_count = value_64;
    }
    // fill the energy consumed
    rsmi_ret = rsmi_dev_energy_count_get(gpu_id, &power, &counter_resolution,
                                         &value_64);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get energy consumed for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->energy_consumed = power * counter_resolution;
    }
    // fill the edge temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_EDGE,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get edge temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.edge_temperature = (float)(temperature / 1000);
    }
    // fill the junction temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_JUNCTION,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get junction temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.junction_temperature = (float)(temperature / 1000);
    }
    // fill the memory temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_MEMORY,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.memory_temperature = (float)(temperature / 1000);
    }
    // fill the HBM0 temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_HBM_0,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM0 temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.hbm_temperature[0] = (float)(temperature / 1000);
    }
    // fill the HBM1 temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_HBM_1,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM1 temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.hbm_temperature[1] = (float)(temperature / 1000);
    }
    // fill the HBM2 temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_HBM_2,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM2 temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.hbm_temperature[2] = (float)(temperature / 1000);
    }
    // fill the HBM3 temperature
    rsmi_ret = rsmi_dev_temp_metric_get(gpu_id, RSMI_TEMP_TYPE_HBM_3,
                                        RSMI_TEMP_CURRENT, &temperature);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM3 temperature for gpu {}, err {}",
                      gpu_id, rsmi_ret);
    } else {
        stats->temperature.hbm_temperature[3] = (float)(temperature / 1000);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_event_read_all (aga_event_read_cb_t cb, void *ctxt)
{
    return g_smi_state.event_read(cb, ctxt);
}

sdk_ret_t
smi_gpu_reset (uint32_t gpu_id, aga_gpu_reset_type_t reset_type)
{
    rsmi_status_t rsmi_ret;
    uint64_t default_power_cap = 0;

    switch(reset_type) {
    case AGA_GPU_RESET_TYPE_NONE:
        // reset GPU itself
        rsmi_ret = rsmi_dev_gpu_reset(gpu_id);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset gpu {}, err {}", gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_CLOCK:
        // reset overdrive
        rsmi_ret = rsmi_dev_overdrive_level_set(gpu_id,
                                                RSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset overdrive, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        // setting perf level to auto seems to be reset clocks as well
        rsmi_ret = rsmi_dev_perf_level_set(gpu_id, RSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset clocks, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_FAN:
        // reset fans
        rsmi_ret = rsmi_dev_fan_reset(gpu_id, 0);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset fans, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_POWER_PROFILE:
        // reset power profile to bootup default
        rsmi_ret = rsmi_dev_power_profile_set(
                        gpu_id, 0, RSMI_PWR_PROF_PRST_BOOTUP_DEFAULT);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset power profile, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        // also reset perf level to auto
        rsmi_ret = rsmi_dev_perf_level_set(gpu_id, RSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset perf level, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_POWER_OVERDRIVE:
        // get default power overdrive
        rsmi_ret = rsmi_dev_power_cap_default_get(gpu_id, &default_power_cap);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get default power cap,  gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        // set power overdrive to default
        rsmi_ret = rsmi_dev_power_cap_set(gpu_id, 0, default_power_cap);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set power cap to default, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_XGMI_ERROR:
        // reset xgmi error status
        rsmi_ret = rsmi_dev_xgmi_error_reset(gpu_id);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset xgmi error status, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_PERF_DETERMINISM:
        // resetting perf level to "auto" resets performance determinism
        rsmi_ret = rsmi_dev_perf_level_set(gpu_id, RSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset perf level, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_COMPUTE_PARTITION:
        // reset compute partition to its boot state
        rsmi_ret = rsmi_dev_compute_partition_reset(gpu_id);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset compute partition state gpu {}, "
                          "err {}", gpu_id, rsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_NPS_MODE:
        // TODO: reset NPS mode
        return SDK_RET_OP_NOT_SUPPORTED;
        break;
    default:
        AGA_TRACE_ERR("unknown reset request for gpu {}", gpu_id);
        return SDK_RET_INVALID_ARG;
    }

    return rsmi_ret_to_sdk_ret(rsmi_ret);
}

static sdk_ret_t
smi_gpu_power_cap_update_ (uint32_t gpu_id, aga_gpu_spec_t *spec)
{
    rsmi_status_t rsmi_ret;
    uint64_t min_power_cap, max_power_cap;

    // 1. get power cap range
    // 2. validate the power cap is within the range
    // 3. set power cap
    // NOTE: power cap 0 indicates reset to default

    // step1: get power cap range
    rsmi_ret = rsmi_dev_power_cap_range_get(gpu_id, 0, &max_power_cap,
                                            &min_power_cap);
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power cap, gpu {}, err {}",
                      gpu_id, rsmi_ret);
        return (rsmi_ret_to_sdk_ret(rsmi_ret));
    }
    // step2: validate power cap
    min_power_cap /= 1000000;
    max_power_cap /= 1000000;
    if ((spec->gpu_power_cap < min_power_cap) ||
        (spec->gpu_power_cap > max_power_cap)) {
        AGA_TRACE_ERR("Power cap {} is out of supported range, gpu {}, "
                      "allowed range {}-{}", spec->gpu_power_cap,
                      gpu_id, min_power_cap, max_power_cap);
        return sdk_ret_t(SDK_RET_INVALID_ARG,
                         ERR_CODE_SMI_GPU_POWER_CAP_OUT_OF_RANGE);
    }
    // step3: set power cap
    rsmi_ret = rsmi_dev_power_cap_set(gpu_id, 0,
                                      (spec->gpu_power_cap * 1000000));
    if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to set power cap, gpu {}, err {}",
                      gpu_id, rsmi_ret);
        return (rsmi_ret_to_sdk_ret(rsmi_ret));
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_update (uint32_t gpu_id, aga_gpu_spec_t *spec, uint64_t upd_mask)
{
    sdk_ret_t ret;
    std::ofstream of;
    std::string dev_path;
    rsmi_status_t rsmi_ret;
    rsmi_clk_type_t clock_type;
    rsmi_dev_perf_level_t perf_level;

    // performance level has to be set to manual (default is auto) to configure
    // the following list of attributes to non default values
    // 1. GPU overdrive level
    // 2. memory overdirve level

    // set performance level to manual if required
    if (upd_mask & AGA_GPU_UPD_OVERDRIVE_LEVEL) {
        rsmi_ret = rsmi_dev_perf_level_get(gpu_id, &perf_level);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get performance level gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        // if performance level is not manual already, set it to manual
        if (perf_level != RSMI_DEV_PERF_LEVEL_MANUAL) {
            rsmi_ret = rsmi_dev_perf_level_set(gpu_id,
                                               RSMI_DEV_PERF_LEVEL_MANUAL);
            if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to set performance level to manual, "
                              "gpu {}, err {}", gpu_id, rsmi_ret);
            }
        }
    }
    // overdrive update
    if (upd_mask & AGA_GPU_UPD_OVERDRIVE_LEVEL) {
        rsmi_ret = rsmi_dev_overdrive_level_set(gpu_id,
                                                spec->overdrive_level);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set overdrive level, gpu {}, err {}",
                          gpu_id, rsmi_ret);
        }
        return (rsmi_ret_to_sdk_ret(rsmi_ret));
    }
    // system clock frequence range update
    if (upd_mask & AGA_GPU_UPD_CLOCK_FREQ_RANGE) {
        for (uint32_t i = 0; i < AGA_GPU_NUM_CFG_CLOCK_TYPES; i++) {
            ret = aga_to_smi_gpu_clock_type(spec->clock_freq[i].clock_type,
                                            &clock_type);
            if (ret != SDK_RET_OK) {
                AGA_TRACE_ERR("Invalid clock type {} specified, gpu {}",
                              spec->clock_freq[i].clock_type, gpu_id);
                return SDK_RET_INVALID_ARG;
            }
            rsmi_ret = rsmi_dev_clk_range_set(gpu_id, spec->clock_freq[i].lo,
                                              spec->clock_freq[i].hi,
                                              clock_type);
            if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to set clock {} frequency range, gpu {}, "
                              "range {}-{}, err {}",
                              spec->clock_freq[i].clock_type, gpu_id,
                              spec->clock_freq[i].lo, spec->clock_freq[i].hi,
                              rsmi_ret);
                return (rsmi_ret_to_sdk_ret(rsmi_ret));
            }
        }
    }
    // power cap update
    if (upd_mask & AGA_GPU_UPD_POWER_CAP) {
        ret = smi_gpu_power_cap_update_(gpu_id, spec);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // performance level update
    if (upd_mask & AGA_GPU_UPD_PERF_LEVEL) {
        perf_level = aga_to_smi_gpu_perf_level(spec->perf_level);
        rsmi_ret = rsmi_dev_perf_level_set(gpu_id, perf_level);
        if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set performance level to {}, "
                          "gpu {}, err {}", perf_level, gpu_id, rsmi_ret);
        }
    }
    // TODO: fan speed update
    // TODO: RAS spec update
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_device_topology (uint32_t gpu_id, aga_device_topology_info_t *info)
{
    uint32_t cnt = 0;
    rsmi_status_t rsmi_ret;
    static std::string name = "GPU";

    for (uint32_t i = 0; i < AGA_MAX_GPU; i++) {
        if ((i != gpu_id) && gpu_db()->find(i)) {
            info->peer_device[cnt].peer_device.type = AGA_DEVICE_TYPE_GPU;
            strcpy(info->peer_device[cnt].peer_device.name,
                   (name + std::to_string(i)).c_str());
            rsmi_ret = rsmi_topo_get_link_type(gpu_id, i,
                           &info->peer_device[cnt].num_hops,
                           (RSMI_IO_LINK_TYPE *)
                               &info->peer_device[cnt].connection.type);
            if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to get link type between gpus {} and {}, "
                              "err {}", gpu_id, i, rsmi_ret);
                // in case of error set num hops to 0xffff and IO link type to
                // none
                info->peer_device[cnt].num_hops = 0xffff;
                info->peer_device[cnt].connection.type = AGA_IO_LINK_TYPE_NONE;
            }
            rsmi_ret = rsmi_topo_get_link_weight(gpu_id, i,
                           &info->peer_device[cnt].link_weight);
            if (unlikely(rsmi_ret != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to get weight for link between gpus {}"
                              "and {}, err {}", gpu_id, i, rsmi_ret);
                // in case of error set link weight to 0xffff
                info->peer_device[cnt].link_weight = 0xffff;
            }
            info->peer_device[cnt].valid = true;
            cnt++;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_discover_gpus (uint32_t *num_gpus, aga_gpu_handle_t *gpu_handles,
                   aga_obj_key_t *gpu_keys)
{
    sdk_ret_t ret;
    uint64_t unique_id;
    rsmi_status_t status;

    if (!num_gpus) {
        return SDK_RET_ERR;
    }
    // get number of GPUs and stash it
    status = rsmi_num_monitor_devices(num_gpus);
    if (unlikely(status != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GPU count, err {}", status);
        return rsmi_ret_to_sdk_ret(status);
    }
    if (gpu_handles) {
        // this is to keep this code equivalent to amdsmi
        for (uint32_t i = 0; i < *num_gpus; i++) {
            gpu_handles[i] = i;
        }
    }
    if (gpu_keys) {
        // compute uuids for each GPU
        for (uint32_t i = 0; i < *num_gpus; i++) {
            status = rsmi_dev_unique_id_get(i, &unique_id);
            if (unlikely(status != RSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to get unique id for GPU {}, err {}",
                              i, status);
                return rsmi_ret_to_sdk_ret(status);
            }
            gpu_keys[i] = gpu_uuid(i, unique_id);
        }
    }
    return SDK_RET_OK;
}

}    // namespace aga
