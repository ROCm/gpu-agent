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


//------------------------------------------------------------------------------

//
// SDK types header file
//------------------------------------------------------------------------------

#ifndef __SDK_TYPES_HPP__
#define __SDK_TYPES_HPP__

#include <cinttypes>
#include <string>
#include <string.h>
#include "include/sdk/globals.hpp"
#include "include/sdk/if.hpp"

#define XCVR_SPROM_SIZE 256

namespace sdk {
namespace types {

enum log_mode_e {
    log_mode_sync     = 0,     // write logs in the context of the caller thread
    log_mode_async    = 1,    // write logs/traces in the context of a backend thread
};

enum syslog_level_e {
    log_none      = 0,
    log_emerg     = 1,
    log_alert     = 2,
    log_crit      = 3,
    log_err       = 4,
    log_warn      = 5,
    log_notice    = 6,
    log_info      = 7,
    log_debug     = 8,
};

typedef uint64_t    mem_addr_t;
/// \brief address structure to capture physical/virtual address mapping info
typedef struct pa_va_map_s {
    /// is_va set to true if address is virtual address
    bool is_va;
    /// physical address (always set even when is_va is true)
    mem_addr_t pa;
    /// virtual address
    mem_addr_t va;
} pa_va_map_t;

typedef enum monitor_type_e {
    MONITOR_TYPE_SYSTEM     = 0,
    MONITOR_TYPE_INTERRUPTS = 1,
} monitor_type_t;

enum class port_link_sm_t {
    PORT_LINK_SM_DISABLED,
    PORT_LINK_SM_ENABLED,

    PORT_LINK_SM_AN_CFG,
    PORT_LINK_SM_AN_DISABLED,
    PORT_LINK_SM_AN_START,
    PORT_LINK_SM_AN_SIGNAL_DETECT,
    PORT_LINK_SM_AN_WAIT_HCD,
    PORT_LINK_SM_AN_COMPLETE,

    PORT_LINK_SM_SERDES_CFG,
    PORT_LINK_SM_WAIT_SERDES_RDY,
    PORT_LINK_SM_MAC_CFG,
    PORT_LINK_SM_SIGNAL_DETECT,

    PORT_LINK_SM_AN_DFE_TUNING,
    PORT_LINK_SM_DFE_TUNING,
    PORT_LINK_SM_DFE_DISABLED,
    PORT_LINK_SM_DFE_START_ICAL,
    PORT_LINK_SM_DFE_WAIT_ICAL,
    PORT_LINK_SM_DFE_START_PCAL,
    PORT_LINK_SM_DFE_WAIT_PCAL,
    PORT_LINK_SM_DFE_PCAL_CONTINUOUS,

    PORT_LINK_SM_CLEAR_MAC_REMOTE_FAULTS,
    PORT_LINK_SM_WAIT_MAC_SYNC,
    PORT_LINK_SM_WAIT_MAC_FAULTS_CLEAR,
    PORT_LINK_SM_WAIT_PHY_LINK_UP,
    PORT_LINK_SM_UP
};

static inline std::string
port_link_sm_to_str (port_link_sm_t state) {
    switch (state) {
    case port_link_sm_t::PORT_LINK_SM_DISABLED:
        return "DISABLED";
    case port_link_sm_t::PORT_LINK_SM_ENABLED:
        return "ENABLED";
    case port_link_sm_t::PORT_LINK_SM_AN_CFG:
        return "AN_CFG";
    case port_link_sm_t::PORT_LINK_SM_AN_DISABLED:
        return "AN_DISABLED";
    case port_link_sm_t::PORT_LINK_SM_AN_START:
        return "AN_START";
    case port_link_sm_t::PORT_LINK_SM_AN_SIGNAL_DETECT:
        return "AN_SIGNAL_DETECT";
    case port_link_sm_t::PORT_LINK_SM_AN_WAIT_HCD:
        return "AN_WAIT_HCD";
    case port_link_sm_t::PORT_LINK_SM_AN_COMPLETE:
        return "AN_COMPLETE";
    case port_link_sm_t::PORT_LINK_SM_SERDES_CFG:
        return "SERDES_CFG";
    case port_link_sm_t::PORT_LINK_SM_WAIT_SERDES_RDY:
        return "WAIT_SERDES_RDY";
    case port_link_sm_t::PORT_LINK_SM_MAC_CFG:
        return "MAC_CFG";
    case port_link_sm_t::PORT_LINK_SM_SIGNAL_DETECT:
        return "SIGNAL_DETECT";
    case port_link_sm_t::PORT_LINK_SM_AN_DFE_TUNING:
        return "AN_DFE_TUNING";
    case port_link_sm_t::PORT_LINK_SM_DFE_TUNING:
        return "DFE_TUNING";
    case port_link_sm_t::PORT_LINK_SM_DFE_DISABLED:
        return "DFE_DISABLED";
    case port_link_sm_t::PORT_LINK_SM_DFE_START_ICAL:
        return "DFE_START_ICAL";
    case port_link_sm_t::PORT_LINK_SM_DFE_WAIT_ICAL:
        return "DFE_WAIT_ICAL";
    case port_link_sm_t::PORT_LINK_SM_DFE_START_PCAL:
        return "DFE_START_PCAL";
    case port_link_sm_t::PORT_LINK_SM_DFE_WAIT_PCAL:
        return "DFE_WAIT_PCAL";
    case port_link_sm_t::PORT_LINK_SM_DFE_PCAL_CONTINUOUS:
        return "DFE_PCAL_CONTINUOUS";
    case port_link_sm_t::PORT_LINK_SM_CLEAR_MAC_REMOTE_FAULTS:
        return "CLEAR_MAC_REMOTE_FAULTS";
    case port_link_sm_t::PORT_LINK_SM_WAIT_MAC_SYNC:
        return "WAIT_MAC_SYNC";
    case port_link_sm_t::PORT_LINK_SM_WAIT_MAC_FAULTS_CLEAR:
        return "WAIT_MAC_FAULTS_CLEAR";
    case port_link_sm_t::PORT_LINK_SM_WAIT_PHY_LINK_UP:
        return "WAIT_PHY_LINK_UP";
    case port_link_sm_t::PORT_LINK_SM_UP:
        return "UP";
    default:
        return "INVALID";
    }
}

enum class port_event_t {
    PORT_EVENT_LINK_NONE   = 0,
    PORT_EVENT_LINK_DOWN   = 1,
    PORT_EVENT_LINK_UP     = 2,
    PORT_EVENT_XCVR_UPDATE = 3
};

typedef enum cable_type_e {
    CABLE_TYPE_NONE,
    CABLE_TYPE_CU,
    CABLE_TYPE_FIBER,
    CABLE_TYPE_MAX
} cable_type_t;

typedef enum port_breakout_mode_e {
    BREAKOUT_MODE_NONE,
    BREAKOUT_MODE_4x25G,
    BREAKOUT_MODE_4x10G,
    BREAKOUT_MODE_2x50G,
    BREAKOUT_MODE_4x50G,
    BREAKOUT_MODE_4x100G,
    BREAKOUT_MODE_2x200G
} port_breakout_mode_t;

typedef enum q_notify_mode_e {
    Q_NOTIFY_MODE_BLOCKING     = 0,
    Q_NOTIFY_MODE_NON_BLOCKING = 1
} q_notify_mode_t;

enum class port_type_t {
    PORT_TYPE_NONE    = 0,
    PORT_TYPE_ETH     = 1,
    PORT_TYPE_MGMT    = 2,
};

enum class port_pause_type_t {
    PORT_PAUSE_TYPE_NONE,  // Disable pause
    PORT_PAUSE_TYPE_LINK,  // Link level pause
    PORT_PAUSE_TYPE_PFC,   // PFC
};

enum class port_loopback_mode_t {
    PORT_LOOPBACK_MODE_NONE,    // Disable loopback
    PORT_LOOPBACK_MODE_MAC,     // MAC loopback
    PORT_LOOPBACK_MODE_PHY,     // PHY/Serdes loopback
};

enum class port_oob_bx_inst_t {
    PORT_OOB_MGMT_BX = 0,       // bx MAC
    PORT_OOB_MTP_BX,
    PORT_OOB_BMC_BX,
};

typedef enum xcvr_state_s {
    XCVR_REMOVED,
    XCVR_INSERTED,
    XCVR_SPROM_PENDING,
    XCVR_SPROM_READ,
    XCVR_SPROM_READ_ERR,
    XCVR_SPROM_READ_NOTIFY,
    XCVR_SPROM_WAIT_MODULE_READY,
} xcvr_state_t;

typedef enum xcvr_type_s {
    XCVR_TYPE_UNKNOWN,
    XCVR_TYPE_SFP,
    XCVR_TYPE_QSFP,
    XCVR_TYPE_QSFP28,
    XCVR_TYPE_QSFP_CMIS,
} xcvr_type_t;

typedef enum xcvr_pid_s {
    XCVR_PID_UNKNOWN,

    // CU
    XCVR_PID_QSFP_100G_CR4,
    XCVR_PID_QSFP_40GBASE_CR4,
    XCVR_PID_SFP_25GBASE_CR_S,
    XCVR_PID_SFP_25GBASE_CR_L,
    XCVR_PID_SFP_25GBASE_CR_N,
    XCVR_PID_QSFP_50G_CR2_FC,
    XCVR_PID_QSFP_50G_CR2,
    XCVR_PID_QSFP_200G_CR4,
    XCVR_PID_QSFP_400G_CR4,

    // Fiber
    XCVR_PID_QSFP_100G_AOC = 50,
    XCVR_PID_QSFP_100G_ACC,
    XCVR_PID_QSFP_100G_SR4,
    XCVR_PID_QSFP_100G_LR4,
    XCVR_PID_QSFP_100G_ER4,
    XCVR_PID_QSFP_40GBASE_ER4,
    XCVR_PID_QSFP_40GBASE_SR4,
    XCVR_PID_QSFP_40GBASE_LR4,
    XCVR_PID_QSFP_40GBASE_AOC,
    XCVR_PID_SFP_25GBASE_SR,
    XCVR_PID_SFP_25GBASE_LR,
    XCVR_PID_SFP_25GBASE_ER,
    XCVR_PID_SFP_25GBASE_AOC,
    XCVR_PID_SFP_10GBASE_SR,
    XCVR_PID_SFP_10GBASE_LR,
    XCVR_PID_SFP_10GBASE_LRM,
    XCVR_PID_SFP_10GBASE_ER,
    XCVR_PID_SFP_10GBASE_AOC,
    XCVR_PID_SFP_10GBASE_CU,
    XCVR_PID_QSFP_100G_CWDM4,
    XCVR_PID_QSFP_100G_PSM4,
    XCVR_PID_SFP_25GBASE_ACC,
    XCVR_PID_SFP_10GBASE_T,
    XCVR_PID_SFP_1000BASE_T,
    XCVR_PID_QSFP_200G_AOC,
    XCVR_PID_QSFP_200G_FR4,
    XCVR_PID_QSFP_200G_DR4,
    XCVR_PID_QSFP_200G_SR4,
    XCVR_PID_QSFP_200G_ACC,
    XCVR_PID_QSFP_400G_FR4,
    XCVR_PID_QSFP_400G_DR4,
    XCVR_PID_QSFP_400G_SR4,
    XCVR_PID_QSFP_400G_VR4,
} xcvr_pid_t;

typedef struct port_an_args_s {
    bool         fec_ability;
    uint32_t     user_cap;
    uint32_t     fec_request;
} __attribute__((packed)) port_an_args_t;

typedef struct xcvr_sprom_data_t {
    float  length_smf_km;
    uint32_t  length_smf;
    uint32_t  length_om1;
    uint32_t  length_om2;
    uint32_t  length_om3;
    float  dac_length;
    uint8_t   vendor_name[16];    // vendor name
    uint8_t   vendor_pn[16];      // vendor part number
    uint8_t   vendor_rev[4];      // vendor revision
    uint8_t   vendor_sn[16];      // serial number
} xcvr_sprom_data_t;

typedef struct xcvr_event_info_s {
    uint32_t          phy_port;       ///< transceiver port number
    uint32_t          ifindex;        ///< ifindex for transceiver port
    xcvr_type_t       type;           ///< transceiver type - SFP/QSFP/QSFP28
    xcvr_state_t      state;          ///< transceiver state - removed, inserted, sprom_read etc
    xcvr_pid_t        pid;            ///< transceiver pid
    cable_type_t      cable_type;     ///< copper or fiber
    port_an_args_t    *port_an_args;  ///< auto-neg params based on cable
    uint8_t           sprom[XCVR_SPROM_SIZE];
} __attribute__((packed)) xcvr_event_info_t;

typedef enum mem_location_s {
    MEM_LOCATION_NONE,
    MEM_LOCATION_LOW,
    MEM_LOCATION_HIGH,
    MEM_LOCATION_DIRECT,
    MEM_LOCATION_MAX
} mem_location_t;

typedef enum mem_type_s {
    MEM_LAYOUT_TYPE_NONE,
    MEM_LAYOUT_TYPE_OS,
    MEM_LAYOUT_TYPE_DATAPATH,
    MEM_LAYOUT_TYPE_SECURED,
    MEM_LAYOUT_TYPE_MAX
} mem_layout_type_t;

typedef struct mem_layout_s {
    mem_addr_t        base_addr;
    mem_addr_t        end_addr;
    void              *va;
    mem_layout_type_t type;
    mem_location_t    location;
} mem_layout_t;

typedef struct dp_mem_layout_s {
    uint64_t     size;                   // size in bytes of memory layout
    uint64_t     lo_base;                // lower non coherent region base
    uint64_t     lo_size;                // lower non coherent region size
    uint64_t     hi_base;                // higher non coherent region base
    uint64_t     hi_size;                // higher non coherent region size
    uint8_t      mem_region_layout_size; // dp memory layout array size
    mem_layout_t *mem_region_layout;     // dp memory layout array per asic
} dp_mem_layout_t;

static inline
std::string xcvrStateToStr (xcvr_state_t state) {
    std::string str;

    switch (state) {
    case XCVR_REMOVED:
        str = "xcvr removed";
        break;
    case XCVR_INSERTED:
        str = "xcvr inserted";
        break;
    case XCVR_SPROM_PENDING:
        str = "xcvr SPROM pending";
        break;
    case XCVR_SPROM_READ:
        str = "xcvr SPROM read";
        break;
    case XCVR_SPROM_READ_ERR:
        str = "xcvr SPROM Read Err";
        break;
    case XCVR_SPROM_READ_NOTIFY:
        str = "xcvr SPROM read notification pending";
        break;
    default:
        str = "xcvr Invalid State";
    }

    return str;
}

// packet type classification
typedef enum pkt_type_e {
    PKT_TYPE_NONE,
    PKT_TYPE_ARP,
    PKT_TYPE_DHCP,
    PKT_TYPE_NDP,
    PKT_TYPE_IPV4,
    PKT_TYPE_IPV6,
} pkt_type_t;

static inline
std::string pkttype2str (pkt_type_t pkt_type) {
    switch (pkt_type) {
    case PKT_TYPE_NONE:
        return "unknown";
    case PKT_TYPE_ARP:
        return "ARP";
    case PKT_TYPE_DHCP:
        return "DHCP";
    case PKT_TYPE_NDP:
        return "NDP";
    case PKT_TYPE_IPV4:
        return "IPV4";
    case PKT_TYPE_IPV6:
        return "IPV6";
    }
    return "invalid pkt type";
}

/// \brief module version
typedef union module_version_s {
        uint32_t version;
        struct {
            uint16_t major;
            uint16_t minor;
        };
} module_version_t;

/// \brief pstate meta information
typedef struct pstate_meta_s {
    pstate_meta_s () {
        version = 0;
        memset(&rsvd, 0, sizeof(rsvd));
    }
    uint32_t version;
    uint64_t rsvd[8];
}  __attribute__((packed)) pstate_meta_t;

/// \brief driver status type i.e. indicates UP/DOWN
typedef enum eth_driver_status_type_s {
    ETH_DRIVER_STATUS_UP,
    ETH_DRIVER_STATUS_DOWN,
} eth_driver_status_type_t;

typedef struct twheel_event_info_s {
    uint32_t lif_id;
    uint64_t twheel_qstate_addr0;
    uint64_t twheel_qstate_addr1;
} twheel_event_info_t;

/// \brief eth driver status event
typedef struct eth_driver_status_event_s {
#define ETH_DRIVER_STATUS_DEV_NAME_SZ   16
#define ETH_DRIVER_STATUS_VER_SZ        128
    int         type;
    char dev_name[ETH_DRIVER_STATUS_DEV_NAME_SZ];
    char driver_ver_str[ETH_DRIVER_STATUS_VER_SZ];
} eth_driver_status_event_t;

/// \brief event definition for sdk
typedef struct sdk_event_s {
    sdk_ipc_event_id_t event_id;
    union {
        xcvr_event_info_t xcvr_event_info;  ///< transceiver event info
        eth_driver_status_event_t driver_status_info;  ///< driver status event
        twheel_event_info_t twheel;
    };
} sdk_event_t;

/// \brief transceiver temperature info
typedef struct xcvr_temperature_s {
    uint32_t temperature;
    uint32_t warning_temperature;
    uint32_t alarm_temperature;
} xcvr_temperature_t;

}    // namespace types
}    // namespace sdk

using sdk::types::port_type_t;
using sdk::types::port_pause_type_t;
using sdk::types::port_event_t;
using sdk::types::port_breakout_mode_t;
using sdk::types::mem_addr_t;
using sdk::types::q_notify_mode_t;
using sdk::types::cable_type_t;
using sdk::types::xcvr_event_info_t;
using sdk::types::xcvr_sprom_data_t;
using sdk::types::port_an_args_t;
using sdk::types::port_link_sm_t;
using sdk::types::port_loopback_mode_t;
using sdk::types::xcvr_type_t;
using sdk::types::xcvr_state_t;
using sdk::types::xcvr_pid_t;
using sdk::types::pkt_type_t;
using sdk::types::monitor_type_t;
using sdk::types::module_version_t;
using sdk::types::pstate_meta_t;
using sdk::types::xcvr_temperature_t;
using sdk::types::eth_driver_status_type_t;
using sdk::types::eth_driver_status_event_t;
using sdk::types::dp_mem_layout_t;
using sdk::types::pa_va_map_t;

#endif    // __SDK_TYPES_HPP__

