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
/// watch attribute list, tier classification and sampling periods shared by
/// all smi backends
///
//----------------------------------------------------------------------------

#include <cstdlib>
#include "nic/gpuagent/api/smi/smi_watch.hpp"

namespace aga {

vector<gpu_watch_field_t> g_watch_field_list;

/// sampling periods in effect, updated on GPU watch object create
static uint32_t g_fast_period_secs = AGA_GPU_WATCH_FAST_PERIOD_DFLT;
static uint32_t g_slow_period_secs = AGA_GPU_WATCH_SLOW_PERIOD_DFLT;

/// \brief    fast period default, overridable by the environment
/// NOTE:
/// retained for deployments that tune the watcher without an RPC; an explicit
/// non-zero period in the GPU watch spec always wins
static uint32_t
smi_watch_fast_period_dflt_ (void)
{
    const char *env = std::getenv("AGA_GPU_WATCH_UPDATE_FREQ");

    if (env != nullptr) {
        int val = atoi(env);
        if ((val >= (int)AGA_GPU_WATCH_FAST_PERIOD_MIN) &&
            (val <= (int)AGA_GPU_WATCH_FAST_PERIOD_MAX)) {
            return (uint32_t)val;
        }
    }
    return AGA_GPU_WATCH_FAST_PERIOD_DFLT;
}

void
smi_watch_set_periods (uint32_t fast_secs, uint32_t slow_secs)
{
    g_fast_period_secs =
        fast_secs ? fast_secs : smi_watch_fast_period_dflt_();
    g_slow_period_secs =
        slow_secs ? slow_secs : AGA_GPU_WATCH_SLOW_PERIOD_DFLT;
}

uint32_t
smi_watch_fast_period (void)
{
    return g_fast_period_secs;
}

uint32_t
smi_watch_slow_period (void)
{
    return g_slow_period_secs;
}

/// attrs served by the single bulk gpu metrics read every watcher tick
static const aga_gpu_watch_attr_id_t k_fast_attrs[] = {
    AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK,
    AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK,
    AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP,
    AGA_GPU_WATCH_ATTR_ID_GPU_TEMP,
    AGA_GPU_WATCH_ATTR_ID_POWER_USAGE,
    AGA_GPU_WATCH_ATTR_ID_GPU_UTIL,
    AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY,
    AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY,
    AGA_GPU_WATCH_ATTR_ID_AVG_POWER,
    AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE,
    AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE,
    AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE,
    AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED,
    // array-valued attrs, all served by the same bulk gpu metrics read
    AGA_GPU_WATCH_ATTR_ID_HBM_TEMP,
    AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY,
    AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY,
    AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX,
    AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST,
    AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST,
    AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST,
    // PCIe accumulated counters + bulk scalars, all from the bulk metrics read
    AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT,
    AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT,
    AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT,
    AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT,
    AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT,
    AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE,
    AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH,
    AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP,
};

/// attrs that each cost their own smi call and so are sampled less often
static const aga_gpu_watch_attr_id_t k_slow_attrs[] = {
    AGA_GPU_WATCH_ATTR_ID_PCIE_TX,
    AGA_GPU_WATCH_ATTR_ID_PCIE_RX,
    AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH,
    AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE,
    AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL,
    AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SDMA,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_GFX,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_HDP,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_DF,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SMN,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SEM,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP0,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP1,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_FUSE,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_UMC,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MCA,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_VCN,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_JPEG,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_IH,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MPIO,
    // the following attributes require that a counter is created and started
    // before amdsmi API calls can be used to read their values; counter is
    // created and started during watcher initialization
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT,
    AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT,
    AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT,
    AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT,
    AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT,
    // violation counters: sourced from a dedicated amdsmi_get_violation_status
    // call, so sampled at the slow period
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT,
    // memory totals/used/free and PCIe link speed: dedicated amdsmi calls
    AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM,
    AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM,
    AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT,
    AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM,
    AGA_GPU_WATCH_ATTR_ID_USED_GTT,
    AGA_GPU_WATCH_ATTR_ID_FREE_VRAM,
    AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM,
    AGA_GPU_WATCH_ATTR_ID_FREE_GTT,
    AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED,
    AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED,
    // clocks: current/min/max assembled together; min/max need per-type
    // amdsmi_get_clock_info / amdsmi_get_clk_freq, so the family is slow tier
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT,
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN,
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX,
};

void
smi_watch_field_list_init (void)
{
    if (!g_watch_field_list.empty()) {
        return;
    }
    for (auto attr_id : k_fast_attrs) {
        g_watch_field_list.push_back({attr_id, GPU_WATCH_TIER_FAST});
    }
    for (auto attr_id : k_slow_attrs) {
        g_watch_field_list.push_back({attr_id, GPU_WATCH_TIER_SLOW});
    }
}

}    // namespace aga
