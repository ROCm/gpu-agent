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
/// This module defines the globals for sdk
///
//----------------------------------------------------------------------------

#ifndef __SDK_GLOBALS_HPP__
#define __SDK_GLOBALS_HPP__

/// \brief ipc endpoint ids reserved for sdk processes
typedef enum sdk_ipc_id_s {
    SDK_IPC_ID_MIN                      = 0,
    SDK_IPC_ID_NONE                     = SDK_IPC_ID_MIN,
    SDK_IPC_ID_UPGMGR                   = SDK_IPC_ID_MIN + 1,
    SDK_IPC_ID_PCIEMGR                  = SDK_IPC_ID_MIN + 2,
    SDK_IPC_ID_OPERD                    = SDK_IPC_ID_MIN + 3,
    SDK_IPC_ID_OPERD_PLUGIN             = SDK_IPC_ID_MIN + 4,
    SDK_IPC_ID_LINKMGR_AACS_SERVER      = SDK_IPC_ID_MIN + 5,
    SDK_IPC_ID_LINKMGR_CTRL             = SDK_IPC_ID_MIN + 6,
    SDK_IPC_ID_SYSMON                   = SDK_IPC_ID_MIN + 7,
    SDK_IPC_ID_NICMGR_MDEV              = SDK_IPC_ID_MIN + 8,
    SDK_IPC_ID_NICMGR_FIRMWARE_INSTALL  = SDK_IPC_ID_MIN + 9,
    SDK_IPC_ID_NICMGR_FIRMWARE_ACTIVATE = SDK_IPC_ID_MIN + 10,
    SDK_IPC_ID_NCSI_CLI                 = SDK_IPC_ID_MIN + 11,
    SDK_IPC_ID_AACS_SERVER              = SDK_IPC_ID_MIN + 12,
    SDK_IPC_ID_HAMGR_MIN                = SDK_IPC_ID_MIN + 13,
    SDK_IPC_ID_HAMGR_MAX                = SDK_IPC_ID_MIN + 14,
    SDK_IPC_ID_PERIODIC                 = SDK_IPC_ID_MIN + 15,
    SDK_IPC_ID_SYSMGR                   = SDK_IPC_ID_MIN + 16,
    SDK_IPC_ID_UPGMGR_SVC               = SDK_IPC_ID_MIN + 17,
    SDK_IPC_ID_OPERD_SVC                = SDK_IPC_ID_MIN + 18,
    SDK_IPC_ID_HAMGR_SVC                = SDK_IPC_ID_MIN + 19,
    SDK_IPC_ID_HAMGR_UPGRADE            = SDK_IPC_ID_MIN + 20,
    SDK_IPC_ID_PEN_OPER_SVC             = SDK_IPC_ID_MIN + 21,
    SDK_IPC_ID_PCIEUTIL                 = SDK_IPC_ID_MIN + 22,
    SDK_IPC_ID_ASIC_DDR_ERR             = SDK_IPC_ID_MIN + 23,
    SDK_IPC_ID_MAX                      = SDK_IPC_ID_MIN + 31,
} sdk_ipc_id_t;

/// \brief ipc broadcast event ids reserved for sdk processes
typedef enum sdk_ipc_event_id_s {
    SDK_IPC_EVENT_ID_MIN                     = 0,
    SDK_IPC_EVENT_ID_UPGMGR_MIN              = SDK_IPC_EVENT_ID_MIN + 1,
    SDK_IPC_EVENT_ID_UPGMGR_MAX              = SDK_IPC_EVENT_ID_MIN + 4,
    SDK_IPC_EVENT_ID_HAMGR_MIN               = SDK_IPC_EVENT_ID_MIN + 5,
    SDK_IPC_EVENT_ID_HAMGR_MAX               = SDK_IPC_EVENT_ID_MIN + 19,
    SDK_IPC_EVENT_ID_HOST_DEV_UP             = SDK_IPC_EVENT_ID_MIN + 20,
    SDK_IPC_EVENT_ID_HOST_DEV_DOWN           = SDK_IPC_EVENT_ID_MIN + 21,
    SDK_IPC_EVENT_ID_XCVR_STATUS             = SDK_IPC_EVENT_ID_MIN + 22,
    SDK_IPC_EVENT_ID_XCVR_DOM_STATUS         = SDK_IPC_EVENT_ID_MIN + 23,
    // event from BIOS Setup Human Interface Infrastructure (HII)
    SDK_IPC_EVENT_ID_HII_UPDATE              = SDK_IPC_EVENT_ID_MIN + 24,
    // event indicating current driver status
    SDK_IPC_EVENT_ID_DRIVER_STATUS           = SDK_IPC_EVENT_ID_MIN + 25,
    // event to indicate all (non-mgmt) uplinks to be brought down by linkmgr
    SDK_IPC_EVENT_ID_ADMIN_LINK_DOWN         = SDK_IPC_EVENT_ID_MIN + 26,
    SDK_IPC_EVENT_ID_SYSMGR_SVC_STATUS       = SDK_IPC_EVENT_ID_MIN + 27,
    SDK_IPC_EVENT_ID_SYSMGR_SVC_RESOURCE_USE = SDK_IPC_EVENT_ID_MIN + 28,
    SDK_IPC_EVENT_ID_SYSMGR_HALT             = SDK_IPC_EVENT_ID_MIN + 29,
    SDK_IPC_EVENT_ID_PMMSG_EVENT             = SDK_IPC_EVENT_ID_MIN + 30,
    SDK_IPC_EVENT_ID_TWHEEL_INIT             = SDK_IPC_EVENT_ID_MIN + 31,
    SDK_IPC_EVENT_ID_MAX                     = SDK_IPC_EVENT_ID_TWHEEL_INIT,
} sdk_ipc_event_id_t;

/// \brief ipc unicast event ids reserved for sdk processes
typedef enum sdk_ipc_msg_id_s {
    SDK_IPC_MSG_ID_MIN            = 0,
    SDK_IPC_MSG_ID_UPGMGR_MIN     = SDK_IPC_MSG_ID_MIN + 1,
    SDK_IPC_MSG_ID_UPGMGR_MAX     = SDK_IPC_MSG_ID_MIN + 20,
    SDK_IPC_MSG_ID_HAMGR_MIN      = SDK_IPC_MSG_ID_MIN + 21,
    SDK_IPC_MSG_ID_HAMGR_MAX      = SDK_IPC_MSG_ID_MIN + 48,
    SDK_IPC_MSG_ID_SYSMON_MIN     = SDK_IPC_MSG_ID_MIN + 49,
    SDK_IPC_MSG_ID_SYSMON_MAX     = SDK_IPC_MSG_ID_MIN + 60,
    SDK_IPC_MSG_ID_PCIEMGR_MIN    = SDK_IPC_MSG_ID_MIN + 61,
    SDK_IPC_MSG_ID_PCIEMGR_MAX    = SDK_IPC_MSG_ID_MIN + 80,
    SDK_IPC_MSG_ID_MAX            = SDK_IPC_MSG_ID_MIN + 81,
} sdk_ipc_msg_id_t;

/// \brief module ids reserved for sdk processes
typedef enum sdk_mod_id_e {
    SDK_MOD_ID_MIN   = 0,
    SDK_MOD_ID_SDK   = SDK_MOD_ID_MIN + 1,  ///< generic SDK module id
    SDK_MOD_ID_HMON,                        ///< module id for system monitoring
    SDK_MOD_ID_TEMP_HMON,                   ///< module id for temperature monitoring
    SDK_MOD_ID_RSC_HMON,                    ///< module id for resource monitoring
    SDK_MOD_ID_INTR,                        ///< module id for system interrupts
    SDK_MOD_ID_INTR_ONETIME,                ///< module id for onetime system interrupts
    SDK_MOD_ID_BOOT,                        ///< module id for boot(reset-reason, ntp time sync) interrupts
    SDK_MOD_ID_LINK,                        ///< module id for port links
    SDK_MOD_ID_UPG,                         ///< module id for upgrade mgr
    SDK_MOD_ID_HA,                          ///< module id for ha
    SDK_MOD_ID_HAL,                         ///< generic module id for HAL
    SDK_MOD_ID_NICMGR,                      ///< generic module id for nicmgr
    SDK_MOD_ID_MAX
} sdk_mod_id_t;

#define SDK_THREAD_ID_PERIODIC        SDK_IPC_ID_PERIODIC

/// global lif id ranges
#define SDK_LIF_ID_MIN                      0                       ///< global lif id min value
#define SDK_LIF_ID_MAX                      2047                    ///< global lif id max value
#define SDK_UPLINK_LIF_ID_MIN               (SDK_LIF_ID_MIN + 1)    ///< uplink lif range min
#define SDK_UPLINK_LIF_ID_MAX               32                      ///< uplink lif range max
#define SDK_SVC_LIF_ID_MIN                  34                      ///< service lif id range min
#define SDK_SVC_LIF_ID_MAX                  64                      ///< service lif id range max

#endif   // __SDK_GLOBALS_HPP__
