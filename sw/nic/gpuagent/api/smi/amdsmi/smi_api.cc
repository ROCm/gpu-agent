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

#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/amdsmi/smi_utils.hpp"

// TODO:
// not using aga_ here for proper naming !!!

namespace aga {

#define AMDSMI_DEEP_SLEEP_THRESHOLD     140

/// \brief struct to be used as ctxt when walking GPU db to build topology
typedef struct gpu_topo_walk_ctxt_s {
    uint32_t count;
    gpu_entry *gpu;
    aga_device_topology_info_t *info;
} gpu_topo_walk_ctxt_t;

/// \brief    fill clock frequency ranges of the given GPU
/// \param[in] gpu_handle    GPU handle
/// \param[out] spec     spec to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_clock_frequency_spec_ (aga_gpu_handle_t gpu_handle,
                                    aga_gpu_spec_t *spec)
{
    uint32_t clk_cnt = 0;
    amdsmi_clk_info_t info;
    amdsmi_status_t amdsmi_ret;
    aga_gpu_clock_freq_range_t *clock_spec;

    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_GFX,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_GFX);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_MEM,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_MEM);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_VCLK0,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get video clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type =
            smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_VCLK0);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCLK0,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get data clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type =
            smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_DCLK0);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    spec->num_clock_freqs = clk_cnt;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_spec (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec)
{
    uint32_t value_32;
    amdsmi_status_t amdsmi_ret;
    char compute_partition_type[AGA_MAX_STR_LEN + 1];
    amdsmi_dev_perf_level_t perf_level;
    amdsmi_power_cap_info_t power_cap_info;

    // fill the overdrive level
    amdsmi_ret = amdsmi_get_gpu_overdrive_level(gpu_handle, &value_32);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get clock overdrive for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        spec->overdrive_level = value_32;
    }
    // fill the power cap
    amdsmi_ret = amdsmi_get_power_cap_info(gpu_handle, 0, &power_cap_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power cap information for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        spec->gpu_power_cap = power_cap_info.power_cap/1000000;
    }
    // fill the perf level
    amdsmi_ret = amdsmi_get_gpu_perf_level(gpu_handle, &perf_level);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get performance level GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        spec->perf_level = smi_to_aga_gpu_perf_level(perf_level);
    }
    // fill the fan speed
    amdsmi_ret = amdsmi_get_gpu_fan_speed(gpu_handle, 0,
                                          (int64_t *)&spec->fan_speed);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get fan speed GPU {}, err {}", gpu_handle,
                      amdsmi_ret);
    }
    // fill gpu and memory clock frequencies
    smi_fill_gpu_clock_frequency_spec_(gpu_handle, spec);
    // fill compute partition type
    amdsmi_ret = amdsmi_get_gpu_compute_partition(gpu_handle,
                     compute_partition_type, AGA_MAX_STR_LEN + 1);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get compute partition for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        spec->compute_partition_type =
            smi_to_aga_gpu_compute_partition_type(compute_partition_type);
    }
    // TODO: get admin_state
    // TODO: get RAS spec
    return SDK_RET_OK;
}

/// \brief     function to get name for amdsmi firmware block enum
/// \param[in] block    amdsmi firmware block enum
/// \return    firmware block name
static inline const char *
gpu_fw_block_name_str_ (amdsmi_fw_block_t block)
{
    switch (block) {
    case AMDSMI_FW_ID_SMU:
        return "SMU";
    case AMDSMI_FW_ID_CP_CE:
        return "CP_CE";
    case AMDSMI_FW_ID_CP_PFP:
        return "CP_PFP";
    case AMDSMI_FW_ID_CP_ME:
        return "CP_ME";
    case AMDSMI_FW_ID_CP_MEC_JT1:
        return "CP_MEC_JT1";
    case AMDSMI_FW_ID_CP_MEC_JT2:
        return "CP_MEC_JT2";
    case AMDSMI_FW_ID_CP_MEC1:
        return "CP_MEC1";
    case AMDSMI_FW_ID_CP_MEC2:
        return "CP_MEC2";
    case AMDSMI_FW_ID_RLC:
        return "RLC";
    case AMDSMI_FW_ID_SDMA0:
        return "SDMA0";
    case AMDSMI_FW_ID_SDMA1:
        return "SDMA1";
    case AMDSMI_FW_ID_SDMA2:
        return "SDMA2";
    case AMDSMI_FW_ID_SDMA3:
        return "SDMA3";
    case AMDSMI_FW_ID_SDMA4:
        return "SDMA4";
    case AMDSMI_FW_ID_SDMA5:
        return "SDMA5";
    case AMDSMI_FW_ID_SDMA6:
        return "SDMA6";
    case AMDSMI_FW_ID_SDMA7:
        return "SDMA7";
    case AMDSMI_FW_ID_VCN:
        return "VCN";
    case AMDSMI_FW_ID_UVD:
        return "UVD";
    case AMDSMI_FW_ID_VCE:
        return "VCE";
    case AMDSMI_FW_ID_ISP:
        return "ISP";
    case AMDSMI_FW_ID_DMCU_ERAM:
        return "DMCU_ERAM";
    case AMDSMI_FW_ID_DMCU_ISR:
        return "DMCU_ISR";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM:
        return "RLC_GPM_MEM";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM:
        return "RLC_SRM_MEM";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL:
        return "RLC_CNTL";
    case AMDSMI_FW_ID_RLC_V:
        return "RLC_V";
    case AMDSMI_FW_ID_MMSCH:
        return "MMSCH";
    case AMDSMI_FW_ID_PSP_SYSDRV:
        return "PSP_SYSDRV";
    case AMDSMI_FW_ID_PSP_SOSDRV:
        return "PSP_SOSDRV";
    case AMDSMI_FW_ID_PSP_TOC:
        return "PSP_TOC";
    case AMDSMI_FW_ID_PSP_KEYDB:
        return "PSP_KEYDB";
    case AMDSMI_FW_ID_DFC:
        return "DFC";
    case AMDSMI_FW_ID_PSP_SPL:
        return "PSP_SPL";
    case AMDSMI_FW_ID_DRV_CAP:
        return "DRV_CAP";
    case AMDSMI_FW_ID_MC:
        return "MC";
    case AMDSMI_FW_ID_PSP_BL:
        return "PSP_BL";
    case AMDSMI_FW_ID_CP_PM4:
        return "CP_PM4";
    case AMDSMI_FW_ID_RLC_P:
        return "RLC_P";
    case AMDSMI_FW_ID_SEC_POLICY_STAGE2:
        return "SEC_POL_STG2";
    case AMDSMI_FW_ID_REG_ACCESS_WHITELIST:
        return "REG_ACCESS_WL";
    case AMDSMI_FW_ID_IMU_DRAM:
        return "IMU_DRAM";
    case AMDSMI_FW_ID_IMU_IRAM:
        return "IMU_IRAM";
    case AMDSMI_FW_ID_SDMA_TH0:
        return "SDMA_TH0";
    case AMDSMI_FW_ID_SDMA_TH1:
        return "SDMA_TH1";
    case AMDSMI_FW_ID_CP_MES:
        return "CP_MES";
    case AMDSMI_FW_ID_MES_KIQ:
        return "MES_KIQ";
    case AMDSMI_FW_ID_MES_STACK:
        return "MES_STACK";
    case AMDSMI_FW_ID_MES_THREAD1:
        return "MES_THREAD1";
    case AMDSMI_FW_ID_MES_THREAD1_STACK:
        return "MES_THREAD1_STACK";
    case AMDSMI_FW_ID_RLX6:
        return "RLX6";
    case AMDSMI_FW_ID_RLX6_DRAM_BOOT:
        return "RLX6_DRAM_BOOT";
    case AMDSMI_FW_ID_RS64_ME:
        return "RS64_ME";
    case AMDSMI_FW_ID_RS64_ME_P0_DATA:
        return "RS64_ME_P0_DATA";
    case AMDSMI_FW_ID_RS64_ME_P1_DATA:
        return "RS64_ME_P1_DATA";
    case AMDSMI_FW_ID_RS64_PFP:
        return "RS64_PFP";
    case AMDSMI_FW_ID_RS64_PFP_P0_DATA:
        return "RS64_PFP_P0_DATA";
    case AMDSMI_FW_ID_RS64_PFP_P1_DATA:
        return "RS64_PFP_P1_DATA";
    case AMDSMI_FW_ID_RS64_MEC:
        return "RS64_MEC";
    case AMDSMI_FW_ID_RS64_MEC_P0_DATA:
        return "RS64_MEC_P0_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P1_DATA:
        return "RS64_MEC_P1_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P2_DATA:
        return "RS64_MEC_P2_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P3_DATA:
        return "RS64_MEC_P3_DATA";
    case AMDSMI_FW_ID_PPTABLE:
        return "PPTABLE";
    case AMDSMI_FW_ID_PSP_SOC:
        return "PSP_SOC";
    case AMDSMI_FW_ID_PSP_DBG:
        return "PSP_DBG";
    case AMDSMI_FW_ID_PSP_INTF:
        return "PSP_INTF";
    case AMDSMI_FW_ID_RLX6_CORE1:
        return "RLX6_CORE1";
    case AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1:
        return "RLX6_DRAM_BOOT_CORE1";
    case AMDSMI_FW_ID_RLCV_LX7:
        return "RLCV_LX7";
    case AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST:
        return "RLC_SAVE_RL";
    case AMDSMI_FW_ID_ASD:
        return "ASD";
    case AMDSMI_FW_ID_TA_RAS:
        return "TA_RAS";
    case AMDSMI_FW_ID_TA_XGMI:
        return "TA_XGMI";
    case AMDSMI_FW_ID_RLC_SRLG:
        return "RLC_SRLG";
    case AMDSMI_FW_ID_RLC_SRLS:
        return "RLC_SRLS";
    case AMDSMI_FW_ID_PM:
        return "PM";
    case AMDSMI_FW_ID_DMCU:
        return "DMCU";
    default:
        return (std::string("FW_ID_")+ std::to_string(block)).c_str();
    }
}

/// \brief      function to format firmware version
/// \param[out] fw_version    firmware component/version after formatting
/// \param[in]  block         firmware component enum
/// \param[in]  version       firmware version
/// \return     none
static void
fill_gpu_fw_version_ (aga_gpu_fw_version_t *fw_version, amdsmi_fw_block_t block,
                      uint64_t version)
{
    char buf[AGA_MAX_STR_LEN + 1];
    std::string block_name = gpu_fw_block_name_str_(block);

    strncpy(fw_version->firmware, block_name.c_str(), AGA_MAX_STR_LEN);
    if ((block == AMDSMI_FW_ID_VCN) || (block == AMDSMI_FW_ID_UVD) ||
        (block == AMDSMI_FW_ID_VCE) ||
        (block == AMDSMI_FW_ID_ASD) || (block == AMDSMI_FW_ID_CP_MES) ||
        (block == AMDSMI_FW_ID_MES_KIQ) || (block == AMDSMI_FW_ID_PSP_SOSDRV)) {
        // 'VCN', 'VCE', 'UVD', 'SOS', 'ASD', 'MES', 'MES KIQ' fw versions
        // needs to hexadecimal
        snprintf(buf, AGA_MAX_STR_LEN, "0x%08" PRIx64, version);
        strncpy(fw_version->version, buf, AGA_MAX_STR_LEN);
    } else if ((block == AMDSMI_FW_ID_TA_XGMI) ||
               (block == AMDSMI_FW_ID_TA_RAS) || (block == AMDSMI_FW_ID_PM)) {
        // TA XGMI, TA RAS, and PM firmware's hex value looks like 0x12345678
        // however, they are parsed as: int(0x12).int(0x34).int(0x56).int(0x78)
        // which results in the following: 12.34.56.78
        unsigned char tmp[8];
        for (auto i = 0; i < 8; i++) {
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
/// \param[in]  vbios    VBIOS part number string
/// \param[out] sku          SKU string dervied from vbios version
/// \return     none
static void
gpu_get_sku_from_vbios_ (char *sku, char *vbios)
{
    char *buf;
    char *token;

    // middle portion in the VBIOS version is SKU XXX-<CARD_SKU>-XXX
    // get first token
    token = strtok_r(vbios, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}", vbios);
        return;
    }
    // second token is the SKU
    token = strtok_r(NULL, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}", vbios);
        return;
    }
    strncpy(sku, token, AGA_MAX_STR_LEN);
}

/// \brief    fill list of pids using the given GPU
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_kfd_pid_status_ (aga_gpu_handle_t gpu_handle,
                              uint32_t gpu_id, aga_gpu_status_t *status)
{
    amdsmi_status_t amdsmi_ret;
    uint32_t gpu_list[AGA_MAX_GPU];
    amdsmi_process_info_t *pid_info;
    uint32_t value_32, num_pid = 0, num_gpus = AGA_MAX_GPU;

    // kernel fusion driver pids
    amdsmi_ret = amdsmi_get_gpu_compute_process_info(NULL, &value_32);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get KFD pid count, err {}", amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    } else {
        // if pid count is non zero, get the pid info
        if (value_32) {
            pid_info =
                (amdsmi_process_info_t *)malloc(sizeof(amdsmi_process_info_t) *
                                                value_32);
            if (pid_info == NULL) {
                AGA_TRACE_ERR("Failed to allocate KFD pid buffer, GPU {}");
                return SDK_RET_OOM;
            }
            amdsmi_ret = amdsmi_get_gpu_compute_process_info(pid_info,
                                                             &value_32);
            if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
                free(pid_info);
                AGA_TRACE_ERR("Failed to get KFD pid info, err {}", amdsmi_ret);
                return amdsmi_ret_to_sdk_ret(amdsmi_ret);
            }
            // loop thru pids, get the list of GPUs using each pid and
            // update per GPU kfd process list
            for (uint32_t i = 0; i < value_32; i++) {
                num_gpus = AGA_MAX_GPU;
                amdsmi_ret =
                    amdsmi_get_gpu_compute_process_gpus(pid_info[i].process_id,
                                                        gpu_list, &num_gpus);
                if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
                    AGA_TRACE_ERR("Failed to get GPU list of pid {}, err {}",
                                  pid_info[i].process_id, amdsmi_ret);
                    continue;
                }
                for (uint32_t j = 0; j < num_gpus; j++) {
                    if (gpu_list[j] == gpu_id) {
                        if (num_pid == (AGA_GPU_MAX_KFD_PID - 1)) {
                            AGA_TRACE_DEBUG("Reached max KFD processes {} "
                                            "using the GPU {}, pid {} is "
                                            "ignored", AGA_GPU_MAX_KFD_PID,
                                            gpu_handle, pid_info[i].process_id);
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
            free(pid_info);
        }
    }
    return SDK_RET_OK;
}

/// \brief    fill status of clocks
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_clock_status_ (aga_gpu_handle_t gpu_handle,
                        aga_gpu_status_t *status,
                        amdsmi_gpu_metrics_t *metrics_info)
{
    uint32_t clk_cnt = 0;
    amdsmi_status_t amdsmi_ret;
    amdsmi_clk_info_t clock_info;
    aga_gpu_clock_status_t *clock_status;

    // fill up information available from metrics info
    if (metrics_info) {
        // gfx clocks
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_GFX_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_SYSTEM;
            clock_status->frequency = metrics_info->current_gfxclks[i];
            clock_status->locked =
                metrics_info->gfxclk_lock_status & (1 << i);
            clock_status->deep_sleep =
                (clock_status->frequency < AMDSMI_DEEP_SLEEP_THRESHOLD);
            clk_cnt++;
        }
        // memory clock
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_MEMORY;
        clock_status->frequency = metrics_info->current_uclk;
        // locked is N/A for memory clock
        clock_status->deep_sleep =
            (clock_status->frequency < AMDSMI_DEEP_SLEEP_THRESHOLD);
        clk_cnt++;
        // video clocks
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_VIDEO;
            clock_status->frequency = metrics_info->current_vclk0s[i];
            // locked is N/A for video clocks
            clock_status->deep_sleep =
                (clock_status->frequency < AMDSMI_DEEP_SLEEP_THRESHOLD);
            clk_cnt++;
        }
        // data clocks
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_DATA;
            clock_status->frequency = metrics_info->current_dclk0s[i];
            // locked is N/A for data clocks
            clock_status->deep_sleep =
                (clock_status->frequency < AMDSMI_DEEP_SLEEP_THRESHOLD);
            clk_cnt++;
        }
    }
    // get additional clock status information from amdsmi_get_clock_info
    clk_cnt = 0;
    // gfx clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_GFX,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_GFX_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->low_frequency = clock_info.min_clk;
            clock_status->high_frequency = clock_info.max_clk;
            clk_cnt++;
        }
    }
    // memory clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_MEM,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clk_cnt++;
    }
    // video clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_VCLK0,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get video clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->low_frequency = clock_info.min_clk;
            clock_status->high_frequency = clock_info.max_clk;
            clk_cnt++;
        }
    }
    // data clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCLK0,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get data clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            clock_status = &status->clock_status[clk_cnt];
            clock_status->low_frequency = clock_info.min_clk;
            clock_status->high_frequency = clock_info.max_clk;
            clk_cnt++;
        }
    }
    // data fabric clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DF,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get fabric clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_FABRIC;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // DCE clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCEF,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get DCE clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_DCE;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // SOC clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_SOC,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get SOC clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_SOC;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // PCIe clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_PCIE,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_PCIE;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    status->num_clock_status = clk_cnt;
    return SDK_RET_OK;
}

/// \brief    fill PCIe status
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_pcie_status_ (aga_gpu_handle_t gpu_handle,
                       aga_gpu_status_t *status)
{
    uint64_t value_64;
    amdsmi_pcie_info_t info;
    amdsmi_status_t amdsmi_ret;
    aga_gpu_pcie_status_t *pcie_status = &status->pcie_status;

    amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        pcie_status->slot_type =
            smi_to_aga_pcie_slot_type(info.pcie_static.slot_type);
        pcie_status->max_width = info.pcie_static.max_pcie_width;
        pcie_status->max_speed = info.pcie_static.max_pcie_speed/1000;
        pcie_status->version = info.pcie_static.pcie_interface_version;
        pcie_status->width = info.pcie_metric.pcie_width;
        pcie_status->speed = info.pcie_metric.pcie_speed/1000;
        pcie_status->bandwidth = info.pcie_metric.pcie_bandwidth;
    }
    amdsmi_ret = amdsmi_get_gpu_bdf_id(gpu_handle, &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe bus id for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
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

/// \brief    fill VRAM status
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_vram_status_ (aga_gpu_handle_t gpu_handle,
                       aga_gpu_vram_status_t *status)
{
    amdsmi_vram_info_t info;
    amdsmi_status_t amdsmi_ret;

    amdsmi_ret = amdsmi_get_gpu_vram_info(gpu_handle, &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        status->type = smi_to_aga_vram_type(info.vram_type);
        status->vendor = smi_to_aga_vram_vendor(info.vram_vendor);
        status->size = info.vram_size;
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_get_gpu_partition_id (aga_gpu_handle_t gpu_handle, uint32_t *partition_id)
{
    amdsmi_status_t status;
    amdsmi_kfd_info_t kfd_info;

    status = amdsmi_get_gpu_kfd_info(gpu_handle, &kfd_info);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get partition id of GPU {}, err {}",
                      gpu_handle, status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    *partition_id = kfd_info.current_partition_id;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_status (aga_gpu_handle_t gpu_handle, uint32_t gpu_id,
                     aga_gpu_status_t *status)
{
    uint32_t i;
    amdsmi_fw_info_t fw_info;
    amdsmi_status_t amdsmi_ret;
    amdsmi_xgmi_status_t xgmi_st;
    amdsmi_vbios_info_t vbios_info;
    amdsmi_board_info_t board_info;
    amdsmi_driver_info_t driver_info;
    amdsmi_od_volt_freq_data_t vc_data;
    amdsmi_gpu_metrics_t metrics_info = { 0 };

    status->index = gpu_id;
    status->handle = gpu_handle;
    // fill the GPU serial number
    amdsmi_ret = amdsmi_get_gpu_board_info(gpu_handle, &board_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
       AGA_TRACE_ERR("Failed to get serial number for GPU {}, err {}",
                     gpu_handle, amdsmi_ret);
    }
    memcpy(status->serial_num, board_info.product_serial, AGA_MAX_STR_LEN);
    // fill the GPU card series
    memcpy(status->card_series, board_info.product_name, AGA_MAX_STR_LEN);
    // fill the GPU vendor information
    memcpy(status->card_vendor, board_info.manufacturer_name, AGA_MAX_STR_LEN);
    // fill the GPU card model
    memcpy(status->card_model, board_info.model_number, AGA_MAX_STR_LEN);
    // fill the driver version
    amdsmi_ret = amdsmi_get_gpu_driver_info(gpu_handle, &driver_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system driver information, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    }
    memcpy(status->driver_version, driver_info.driver_version, AGA_MAX_STR_LEN);

    // fill the vbios version
    amdsmi_ret = amdsmi_get_gpu_vbios_info(gpu_handle, &vbios_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get vbios version for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        strncpy(status->vbios_version, vbios_info.version, AGA_MAX_STR_LEN);
        strncpy(status->vbios_part_number, vbios_info.part_number,
                AGA_MAX_STR_LEN);
        // sku should be retrieved from vbios version
        gpu_get_sku_from_vbios_(status->card_sku, vbios_info.part_number);
    }
    // fill the firmware version
    amdsmi_ret = amdsmi_get_fw_info(gpu_handle, &fw_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get firmware version for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        memset(status->fw_version, 0,
               sizeof(aga_gpu_fw_version_t) * AGA_GPU_MAX_FIRMWARE_VERSION);
        for (i = 0; i < fw_info.num_fw_info; i++) {
            fill_gpu_fw_version_(&status->fw_version[i],
                                 fw_info.fw_info_list[i].fw_id,
                                 fw_info.fw_info_list[i].fw_version);
        }
        status->num_fw_versions = fw_info.num_fw_info;
    }
    // fill the memory vendor
    amdsmi_ret =  amdsmi_get_gpu_vram_vendor(gpu_handle, status->memory_vendor,
                                             AGA_MAX_STR_LEN);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory vendor for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    }
    amdsmi_ret = amdsmi_get_gpu_metrics_info(gpu_handle, &metrics_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GPU metrics info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        // fill the clock status without metrics info
        smi_fill_clock_status_(gpu_handle, status, NULL);
    } else {
        // fill the clock status with metrics info
        smi_fill_clock_status_(gpu_handle, status, &metrics_info);
        // fill firmware timestamp
        status->fw_timestamp = metrics_info.firmware_timestamp;
        if (metrics_info.throttle_status !=
            std::numeric_limits<uint32_t>::max()) {
            status->throttling_status =
                metrics_info.throttle_status ? AGA_GPU_THROTTLING_STATUS_ON :
                                               AGA_GPU_THROTTLING_STATUS_OFF;
        }
        status->xgmi_status.width = metrics_info.xgmi_link_width;
        status->xgmi_status.speed = metrics_info.xgmi_link_speed;
    }
    // fill the PCIe status
    smi_fill_pcie_status_(gpu_handle, status);
    // fill VRAM status
    smi_fill_vram_status_(gpu_handle, &status->vram_status);
    // fill the xgmi error count
    amdsmi_ret = amdsmi_gpu_xgmi_error_status(gpu_handle, &xgmi_st);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get xgmi error status for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        status->xgmi_status.error_status = smi_to_aga_gpu_xgmi_error(xgmi_st);
    }
    // fill the voltage curve points
    amdsmi_ret = amdsmi_get_gpu_od_volt_info(gpu_handle, &vc_data);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get voltage curve points for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        for (uint32_t i = 0;
             (i < AGA_GPU_MAX_VOLTAGE_CURVE_POINT) &&
             (i < AMDSMI_NUM_VOLTAGE_CURVE_POINTS); i++) {
            status->voltage_curve_point[i].point = i;
            status->voltage_curve_point[i].frequency =
                vc_data.curve.vc_points[i].frequency/1000000;
            status->voltage_curve_point[i].voltage =
                vc_data.curve.vc_points[i].voltage;
        }
    }
    // fill partition id
    smi_get_gpu_partition_id(gpu_handle, &status->partition_id);
    smi_fill_gpu_kfd_pid_status_(gpu_handle, gpu_id, status);
    // TODO: oper status
    // TODO: RAS status
    return SDK_RET_OK;
}

/// \brief function to get number of bad pages for GPU
/// \param[in]  gpu             GPU object
/// \param[out] num_bad_pages   number of bad pages
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t
smi_gpu_get_bad_page_count (void *gpu_obj,
                            uint32_t *num_bad_pages)
{
    amdsmi_status_t amdsmi_ret;
    gpu_entry *gpu = (gpu_entry *)gpu_obj;

    // get number of bad page records
    amdsmi_ret = amdsmi_get_gpu_bad_page_info(gpu->handle(),
                                              num_bad_pages, NULL);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get bad page information for GPU {}, err {}",
                      gpu->handle(), amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    }
    return SDK_RET_OK;
}

/// \brief function to get GPU bad page records
/// \param[in]  gpu           GPU object
/// \param[in]  num_bad_pages number of bad pages
/// \param[out] records       GPU bad page records
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t
smi_gpu_get_bad_page_records (void *gpu_obj,
                              uint32_t num_bad_pages,
                              aga_gpu_bad_page_record_t *records)
{
    amdsmi_status_t amdsmi_ret;
    gpu_entry *gpu = (gpu_entry *)gpu_obj;
    amdsmi_retired_page_record_t *bad_pages;

    // allocate memory for bad pages
    bad_pages =
        (amdsmi_retired_page_record_t *)malloc(
            num_bad_pages * sizeof(amdsmi_retired_page_record_t));
    if (!bad_pages) {
        AGA_TRACE_ERR("Failed to allocate memory for bad page information "
                      "for GPU {}", gpu->key().str());
        return SDK_RET_OOM;
    }
    // fill bad page records
    amdsmi_ret = amdsmi_get_gpu_bad_page_info(gpu->handle(), &num_bad_pages,
                                              bad_pages);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get bad page information for GPU {}, "
                      "err {}", gpu->handle(), amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    } else {
        for (uint32_t i = 0; i < num_bad_pages; i ++) {
            records[i].key = gpu->key();
            records[i].page_address = bad_pages[i].page_address;
            records[i].page_size = bad_pages[i].page_size;
            records[i].page_status =
                smi_to_aga_gpu_page_status(bad_pages[i].status);
        }
    }
    // free memory
    free(bad_pages);
    return SDK_RET_OK;
}

static sdk_ret_t
smi_fill_vram_usage_ (aga_gpu_handle_t gpu_handle,
                      aga_gpu_vram_usage_t *usage)
{
    uint64_t value_64;
    amdsmi_status_t amdsmi_ret;

    memset(usage, 0, sizeof(aga_gpu_vram_usage_t));
    amdsmi_ret = amdsmi_get_gpu_memory_total(gpu_handle, AMDSMI_MEM_TYPE_VRAM,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM total memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->total_vram = value_64/1024/1024;
    }
    amdsmi_ret = amdsmi_get_gpu_memory_total(gpu_handle,
                                             AMDSMI_MEM_TYPE_VIS_VRAM,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get visible VRAM total memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->total_visible_vram = value_64/1024/1024;
    }
    amdsmi_ret = amdsmi_get_gpu_memory_total(gpu_handle,
                                             AMDSMI_MEM_TYPE_GTT,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GTT total memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->total_gtt = value_64/1024/1024;
    }
    amdsmi_ret = amdsmi_get_gpu_memory_usage(gpu_handle, AMDSMI_MEM_TYPE_VRAM,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM used memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->used_vram = value_64/1024/1024;
    }
    amdsmi_ret = amdsmi_get_gpu_memory_usage(gpu_handle,
                                             AMDSMI_MEM_TYPE_VIS_VRAM,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get visible VRAM used memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->used_visible_vram = value_64/1024/1024;
    }
    amdsmi_ret = amdsmi_get_gpu_memory_usage(gpu_handle,
                                             AMDSMI_MEM_TYPE_GTT,
                                             &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GTT used memory GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        usage->used_gtt = value_64/1024/1024;
    }
    usage->free_vram = usage->total_vram - usage->used_vram;
    usage->free_visible_vram = usage->total_visible_vram -
                                   usage->used_visible_vram;
    usage->free_gtt = usage->total_gtt - usage->used_gtt;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_stats (aga_gpu_handle_t gpu_handle, aga_gpu_stats_t *stats)
{
    int64_t temperature;
    uint64_t power, value_64;
    float counter_resolution;
    amdsmi_status_t amdsmi_ret;
    amdsmi_pcie_info_t pcie_info;
    amdsmi_power_info_t power_info;
    amdsmi_engine_usage_t usage_info;
    amdsmi_gpu_metrics_t metrics_info;

    // fill the power and voltage info
    amdsmi_ret = amdsmi_get_power_info(gpu_handle, &power_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power information for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->avg_package_power = power_info.average_socket_power;
        stats->package_power = power_info.current_socket_power;
        stats->voltage.voltage = power_info.soc_voltage;
        stats->voltage.gfx_voltage = power_info.gfx_voltage;
        stats->voltage.memory_voltage = power_info.mem_voltage;
    }
    // fill the GPU usage
    amdsmi_ret = amdsmi_get_gpu_activity(gpu_handle, &usage_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GPU activity for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->usage.gfx_activity = usage_info.gfx_activity;
        stats->usage.umc_activity = usage_info.umc_activity;
        stats->usage.mm_activity = usage_info.mm_activity;
    }
    // fill VRAM usage
    smi_fill_vram_usage_(gpu_handle, &stats->vram_usage);
    amdsmi_ret = amdsmi_get_gpu_metrics_info(gpu_handle, &metrics_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GPU metrics info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->usage.num_vcn = AMDSMI_MAX_NUM_VCN;
        for (uint16_t i = 0; i < stats->usage.num_vcn; i++) {
            stats->usage.vcn_activity[i] = metrics_info.vcn_activity[i];
        }
        stats->usage.num_jpeg = AMDSMI_MAX_NUM_JPEG;
        for (uint16_t i = 0; i < stats->usage.num_jpeg; i++) {
            stats->usage.jpeg_activity[i] = metrics_info.jpeg_activity[i];
        }
        stats->fan_speed = metrics_info.current_fan_speed;
        stats->gfx_activity_accumulated = metrics_info.gfx_activity_acc;
        stats->mem_activity_accumulated = metrics_info.mem_activity_acc;
        for (uint32_t i = 0; i < AGA_GPU_MAX_XGMI_LINKS; i++) {
            stats->xgmi_link_stats[i].data_read =
                metrics_info.xgmi_read_data_acc[i];
            stats->xgmi_link_stats[i].data_write =
                metrics_info.xgmi_write_data_acc[i];
        }
        // fill violation statistics
        stats->violation_stats.current_accumulated_counter =
            metrics_info.accumulation_counter;
        stats->violation_stats.processor_hot_residency_accumulated =
            metrics_info.prochot_residency_acc;
        stats->violation_stats.ppt_residency_accumulated =
            metrics_info.ppt_residency_acc;
        stats->violation_stats.socket_thermal_residency_accumulated =
            metrics_info.socket_thm_residency_acc;
        stats->violation_stats.vr_thermal_residency_accumulated =
            metrics_info.vr_thm_residency_acc;
        stats->violation_stats.hbm_thermal_residency_accumulated =
            metrics_info.hbm_thm_residency_acc;

    }
    // fill the PCIe stats
    amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &pcie_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->pcie_stats.replay_count =
            pcie_info.pcie_metric.pcie_replay_count;
        stats->pcie_stats.recovery_count =
            pcie_info.pcie_metric.pcie_l0_to_recovery_count;
        stats->pcie_stats.replay_rollover_count =
            pcie_info.pcie_metric.pcie_replay_roll_over_count;
        stats->pcie_stats.nack_sent_count =
            pcie_info.pcie_metric.pcie_nak_sent_count;
        stats->pcie_stats.nack_received_count =
            pcie_info.pcie_metric.pcie_nak_received_count;
    }
    // fill the energy consumed
    amdsmi_ret = amdsmi_get_energy_count(gpu_handle, &power,
                                         &counter_resolution, &value_64);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get energy consumed for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->energy_consumed = power * counter_resolution;
    }
    // fill the edge temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_EDGE,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get edge temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.edge_temperature = (float)temperature;
    }
    // fill the junction temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_JUNCTION,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get junction temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.junction_temperature = (float)temperature;
    }
    // fill the memory temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_VRAM,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.memory_temperature = (float)temperature;
    }
    // fill the HBM0 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_0,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM0 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[0] = (float)temperature;
    }
    // fill the HBM1 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_1,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM1 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[1] = (float)temperature;
    }
    // fill the HBM2 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_2,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM2 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[2] = (float)temperature;
    }
    // fill the HBM3 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_3,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM3 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[3] = (float)temperature;
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_event_read_all (aga_event_read_cb_t cb, void *ctxt)
{
    return g_smi_state.event_read(cb, ctxt);
}

sdk_ret_t
smi_gpu_reset (aga_gpu_handle_t gpu_handle,
               aga_gpu_reset_type_t reset_type)
{
    amdsmi_status_t amdsmi_ret;
    amdsmi_power_cap_info_t power_cap_info;

    switch(reset_type) {
    case AGA_GPU_RESET_TYPE_NONE:
        // reset GPU itself
        amdsmi_ret = amdsmi_reset_gpu(gpu_handle);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset GPU {}, err {}", gpu_handle,
                          amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_CLOCK:
        // reset overdrive
        amdsmi_ret = amdsmi_set_gpu_overdrive_level(gpu_handle,
                                                    AMDSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset overdrive, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        // setting perf level to auto seems to be reset clocks as well
        amdsmi_ret = amdsmi_set_gpu_perf_level(gpu_handle,
                                               AMDSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset clocks, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_FAN:
        // reset fans
        amdsmi_ret = amdsmi_reset_gpu_fan(gpu_handle, 0);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset fans, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_POWER_PROFILE:
        // reset power profile to bootup default
        amdsmi_ret = amdsmi_set_gpu_power_profile(gpu_handle, 0,
                         AMDSMI_PWR_PROF_PRST_BOOTUP_DEFAULT);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset power profile, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        // also reset perf level to auto
        amdsmi_ret = amdsmi_set_gpu_perf_level(gpu_handle,
                                               AMDSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset perf level, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_POWER_OVERDRIVE:
        // get default power overdrive
        amdsmi_ret = amdsmi_get_power_cap_info(gpu_handle, 0,
                                               &power_cap_info);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get default power cap,  GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        // set power overdrive to default
        amdsmi_ret = amdsmi_set_power_cap(gpu_handle, 0,
                                          power_cap_info.default_power_cap);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set power cap to default, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_XGMI_ERROR:
        // reset xgmi error status
        amdsmi_ret = amdsmi_reset_gpu_xgmi_error(gpu_handle);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset xgmi error status, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_PERF_DETERMINISM:
        // resetting perf level to "auto" resets performance determinism
        amdsmi_ret = amdsmi_set_gpu_perf_level(gpu_handle,
                                               AMDSMI_DEV_PERF_LEVEL_AUTO);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to reset perf level, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        break;
    case AGA_GPU_RESET_TYPE_COMPUTE_PARTITION:
        // TODO: reset partition not yet support by amd-smi
        return SDK_RET_OP_NOT_SUPPORTED;
        break;
    case AGA_GPU_RESET_TYPE_NPS_MODE:
        // TODO: reset NPS mode
        return SDK_RET_OP_NOT_SUPPORTED;
        break;
    default:
        AGA_TRACE_ERR("unknown reset request for GPU {}", gpu_handle);
        return SDK_RET_INVALID_ARG;
    }

    return amdsmi_ret_to_sdk_ret(amdsmi_ret);
}

static sdk_ret_t
smi_gpu_power_cap_update_ (aga_gpu_handle_t gpu_handle,
                           aga_gpu_spec_t *spec)
{
    amdsmi_status_t amdsmi_ret;
    amdsmi_power_cap_info_t power_cap_info;

    // 1. get power cap range
    // 2. validate the power cap is within the range
    // 3. set power cap
    // NOTE: power cap 0 indicates reset to default

    // step1: get power cap range
    amdsmi_ret = amdsmi_get_power_cap_info(gpu_handle, 0, &power_cap_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power cap, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    // step2: validate power cap
    power_cap_info.min_power_cap /= 1000000;
    power_cap_info.max_power_cap /= 1000000;
    if ((spec->gpu_power_cap < power_cap_info.min_power_cap) ||
        (spec->gpu_power_cap > power_cap_info.max_power_cap)) {
        AGA_TRACE_ERR("Power cap {} is out of supported range, GPU {}, "
                      "allowed range {}-{}", spec->gpu_power_cap,
                      gpu_handle, power_cap_info.min_power_cap,
                      power_cap_info.max_power_cap);
        return sdk_ret_t(SDK_RET_INVALID_ARG,
                         ERR_CODE_SMI_GPU_POWER_CAP_OUT_OF_RANGE);
    }
    // step3: set power cap
    amdsmi_ret = amdsmi_set_power_cap(gpu_handle, 0,
                                      (spec->gpu_power_cap * 1000000));
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to set power cap, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_update (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec,
                uint64_t upd_mask)
{
    sdk_ret_t ret;
    std::ofstream of;
    std::string dev_path;
    amdsmi_status_t amdsmi_ret;
    amdsmi_clk_type_t clock_type;
    amdsmi_dev_perf_level_t perf_level;

    // performance level has to be set to manual (default is auto) to configure
    // the following list of attributes to non default values
    // 1. GPU overdrive level
    // 2. memory overdirve level

    // set performance level to manual if required
    if (upd_mask & AGA_GPU_UPD_OVERDRIVE_LEVEL) {
        amdsmi_ret = amdsmi_get_gpu_perf_level(gpu_handle, &perf_level);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get performance level GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        // if performance level is not manual already, set it to manual
        if (perf_level != AMDSMI_DEV_PERF_LEVEL_MANUAL) {
            amdsmi_ret = amdsmi_set_gpu_perf_level(gpu_handle,
                             AMDSMI_DEV_PERF_LEVEL_MANUAL);
            if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to set performance level to manual, "
                              "GPU {}, err {}", gpu_handle, amdsmi_ret);
            }
        }
    }
    // overdrive update
    if (upd_mask & AGA_GPU_UPD_OVERDRIVE_LEVEL) {
        amdsmi_ret = amdsmi_set_gpu_overdrive_level(gpu_handle,
                                                    spec->overdrive_level);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set overdrive level, GPU {}, err {}",
                          gpu_handle, amdsmi_ret);
        }
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    // system clock frequence range update
    if (upd_mask & AGA_GPU_UPD_CLOCK_FREQ_RANGE) {
        for (uint32_t i = 0; i < AGA_GPU_NUM_CFG_CLOCK_TYPES; i++) {
            ret = aga_to_smi_gpu_clock_type(spec->clock_freq[i].clock_type,
                                            &clock_type);
            if (ret != SDK_RET_OK) {
                AGA_TRACE_ERR("Invalid clock type {} specified, GPU {}",
                              spec->clock_freq[i].clock_type, gpu_handle);
                return SDK_RET_INVALID_ARG;
            }
            amdsmi_ret = amdsmi_set_gpu_clk_range(gpu_handle,
                             spec->clock_freq[i].lo, spec->clock_freq[i].hi,
                             clock_type);
            if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to set clock {} frequency range, GPU {}, "
                              "range {}-{}, err {}",
                              spec->clock_freq[i].clock_type, gpu_handle,
                              spec->clock_freq[i].lo, spec->clock_freq[i].hi,
                              amdsmi_ret);
                return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
            }
        }
    }
    // power cap update
    if (upd_mask & AGA_GPU_UPD_POWER_CAP) {
        ret = smi_gpu_power_cap_update_(gpu_handle, spec);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // performance level update
    if (upd_mask & AGA_GPU_UPD_PERF_LEVEL) {
        perf_level = aga_to_smi_gpu_perf_level(spec->perf_level);
        amdsmi_ret = amdsmi_set_gpu_perf_level(gpu_handle, perf_level);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set performance level to {}, "
                          "GPU {}, err {}", perf_level, gpu_handle, amdsmi_ret);
        }
    }
    // fan speed update
    if (upd_mask & AGA_GPU_UPD_FAN_SPEED) {
        amdsmi_ret = amdsmi_set_gpu_fan_speed(gpu_handle, 0,
                                              (int64_t)spec->fan_speed);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set fan speed to {}, GPU {}, err {}",
                          spec->fan_speed, gpu_handle, amdsmi_ret);
        }
    }
    // TODO: RAS spec update
    return SDK_RET_OK;
}

/// \brief  callback function to be used to fill topology information between
///         two GPUS
/// \param[in]  obj     GPU object returned by walk function
/// \param[in]  ctxt    opaque context passed to the callback function
/// \return false in case walk should continue or true otherwise
static inline bool
gpu_topo_walk_cb (void *obj, void *ctxt)
{
    gpu_entry *gpu1, *gpu2;
    amdsmi_status_t amdsmi_ret;
    static std::string name = "GPU";
    gpu_topo_walk_ctxt_t *walk_ctxt;
    aga_device_topology_info_t *info;

    gpu2 = (gpu_entry *)obj;
    walk_ctxt = (gpu_topo_walk_ctxt_t *)ctxt;
    gpu1 = walk_ctxt->gpu;
    info = walk_ctxt->info;

    if (gpu1->handle() != gpu2->handle()) {
        info->peer_device[walk_ctxt->count].peer_device.type =
            AGA_DEVICE_TYPE_GPU;
        strcpy(info->peer_device[walk_ctxt->count].peer_device.name,
               (name + std::to_string(gpu1->id())).c_str());
        amdsmi_ret =
            amdsmi_topo_get_link_type(gpu1->handle(), gpu2->handle(),
                &info->peer_device[walk_ctxt->count].num_hops,
                (amdsmi_io_link_type_t *)
                     &info->peer_device[walk_ctxt->count].connection.type);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get link type between gpus {} and {}, "
                          "err {}", gpu1->handle(), gpu2->handle(), amdsmi_ret);
            // in case of error set num hops to 0xffff and IO link type to
            // none
            info->peer_device[walk_ctxt->count].num_hops = 0xffff;
            info->peer_device[walk_ctxt->count].connection.type =
                AGA_IO_LINK_TYPE_NONE;
        }
        amdsmi_ret = amdsmi_topo_get_link_weight(gpu1->handle(), gpu2->handle(),
                         &info->peer_device[walk_ctxt->count].link_weight);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get weight for link between gpus {}"
                          "and {}, err {}", gpu1->handle(), gpu2->handle(),
                          amdsmi_ret);
            // in case of error set link weight to 0xffff
            info->peer_device[walk_ctxt->count].link_weight = 0xffff;
        }
        info->peer_device[walk_ctxt->count].valid = true;
        walk_ctxt->count++;
    }
    return false;
}

sdk_ret_t
smi_gpu_fill_device_topology (aga_gpu_handle_t gpu_handle,
                              aga_device_topology_info_t *info)
{
    gpu_entry *gpu;
    gpu_topo_walk_ctxt_t ctxt;

    gpu = gpu_db()->find(gpu_handle);
    if (gpu == NULL) {
        AGA_TRACE_ERR("Failed to find GPU {}", gpu_handle);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    ctxt.count = 0;
    ctxt.info = info;
    ctxt.gpu = gpu;

    // walk gpu db and fill device topology
    gpu_db()->walk_handle_db(gpu_topo_walk_cb, &ctxt);
    return SDK_RET_OK;
}

/// \brief function to get aga_obj_key_t for a given GPU
/// \param[in]  gpu_handle  GPU handle
/// \param[out] key         aga_obj_key_t of the GPU
static sdk_ret_t
smi_gpu_uuid_get (aga_gpu_handle_t gpu_handle, aga_obj_key_t *key)
{
    amdsmi_status_t status;
    char uuid_rem[20];
    char uuid[AMDSMI_GPU_UUID_SIZE];
    uint32_t uuid_len = AMDSMI_GPU_UUID_SIZE;

    // get uuid from amdsmi
    status = amdsmi_get_gpu_device_uuid(gpu_handle, &uuid_len, uuid);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get uuid of GPU {}, err {}",
                      gpu_handle, status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // amdsmi returns a string containing the uuid of the GPU (ex:
    // 2eff74a1-0000-1000-80fe-9cea14a6b148); to derive the aga_obj_key_t from
    // it we scan the string and construct our aga_obj_key_t
    sscanf(uuid, "%x-%hx-%hx-%hx-%s", (uint32_t *)&key->id[0],
           (uint16_t *)&key->id[4], (uint16_t *)&key->id[6],
           (uint16_t *)&key->id[8], uuid_rem);
    *(uint32_t *)&key->id[0] = htonl(*(uint32_t *)&key->id[0]);
    *(uint16_t *)&key->id[4] = htons(*(uint16_t *)&key->id[4]);
    *(uint16_t *)&key->id[6] = htons(*(uint16_t *)&key->id[6]);
    *(uint16_t *)&key->id[8] = htons(*(uint16_t *)&key->id[8]);
    sscanf(uuid_rem, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", &key->id[10],
           &key->id[11], &key->id[12], &key->id[13], &key->id[14],
           &key->id[15]);
    return SDK_RET_OK;
}

sdk_ret_t
smi_discover_gpus (uint32_t *num_gpus, aga_gpu_handle_t *gpu_handles,
                   aga_obj_key_t *gpu_keys)
{
    sdk_ret_t ret;
    uint32_t num_procs;
    uint32_t num_sockets;
    amdsmi_status_t status;
    processor_type_t proc_type;
    amdsmi_socket_handle socket_handles[AGA_MAX_SOCKET];
    aga_gpu_handle_t proc_handles[AGA_MAX_PROCESSORS_PER_SOCKET];

    if (!num_gpus) {
        return SDK_RET_ERR;
    }
    *num_gpus = 0;
    // get the socket count available in the system
    status = amdsmi_get_socket_handles(&num_sockets, NULL);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get number of sockets from amd smi library, "
                      "err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // get the socket handles in the system
    status = amdsmi_get_socket_handles(&num_sockets, &socket_handles[0]);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get socket handles from amd smi library, "
                      "err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    for (uint32_t i = 0; i < num_sockets; i++) {
        // for each socket get the number of processors
        status = amdsmi_get_processor_handles(socket_handles[i],
                                              &num_procs, NULL);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get number of processors in socket handle "
                          "{} from amd smi library, err {}", socket_handles[i],
                          status);
            return amdsmi_ret_to_sdk_ret(status);
        }
        // for each socket get the processor handles
        status = amdsmi_get_processor_handles(socket_handles[i],
                                              &num_procs, &proc_handles[0]);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get processor handles in socket handle "
                          "{} from amd smi library, err {}", socket_handles[i],
                          status);
            return amdsmi_ret_to_sdk_ret(status);
        }
        // get uuids of each GPU
        for (uint32_t j = 0; j < num_procs; j++) {
            status = amdsmi_get_processor_type(proc_handles[j], &proc_type);
            if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
                AGA_TRACE_ERR("Failed to get processor type of processor {}"
                              " from amd smi library, err {}", proc_handles[j],
                              status);
                return amdsmi_ret_to_sdk_ret(status);
            }
            if (proc_type == AMDSMI_PROCESSOR_TYPE_AMD_GPU) {
                gpu_handles[*num_gpus] = proc_handles[j];
                if (gpu_keys) {
                    ret = smi_gpu_uuid_get(proc_handles[j],
                                           &gpu_keys[*num_gpus]);
                    if (ret != SDK_RET_OK) {
                        AGA_TRACE_ERR("GPU discovery failed due to error in "
                                      "getting UUID of GPU {}",
                                      proc_handles[j]);
                        return ret;
                    }
                }
                (*num_gpus)++;
            }
        }
    }
    return SDK_RET_OK;
}

}    // namespace aga
