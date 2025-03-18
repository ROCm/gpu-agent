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
/// common interface APIs & macros
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_IF_H__
#define __INCLUDE_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum if_type_e {
    IF_TYPE_NONE            = 0,
    IF_TYPE_ETH             = 1,
    IF_TYPE_ETH_PC          = 2,
    IF_TYPE_TUNNEL          = 3,
    IF_TYPE_UPLINK          = 4,
    IF_TYPE_UPLINK_PC       = 5,
    IF_TYPE_L3              = 6,
    IF_TYPE_LIF             = 7,
    IF_TYPE_LOOPBACK        = 8,
    IF_TYPE_CONTROL         = 9,
    IF_TYPE_HOST            = 10,
    IF_TYPE_MGMT_INTERNAL   = 11,
    IF_TYPE_DPA             = 12,
    IF_TYPE_CRYPTO          = 13,
} if_type_t;

typedef uint32_t if_index_t;     /// interface index

// maximum no. of physical ports
#define MAX_PHY_PORT              2

#define LINKMGR_MAX_PORT_LANES    4
#define IFINDEX_INVALID           0x0

#define IF_TYPE_SHIFT             28
#define IF_TYPE_MASK              0xF
#define ETH_IF_SLOT_SHIFT         24
#define ETH_IF_PARENT_PORT_SHIFT  16
#define ETH_IF_SLOT_MASK          0xF
#define ETH_IF_PARENT_PORT_MASK   0xFF
#define ETH_IF_CHILD_PORT_MASK    0xFFFF
#define ETH_IF_DEFAULT_CHILD_PORT 0
#define ETH_IF_DEFAULT_SLOT       1
#define LIF_IF_LIF_ID_MASK        0xFFFFFF
#define LOOPBACK_IF_ID_MASK       0xFFFFFF
#define LOOPBACK_IF_ID_MASK       0xFFFFFF
#define HOST_IF_ID_MASK           0xFFFFFF
#define DPA_IF_ID_MASK            0xFFFFFF
#define CRYPTO_IF_ID_MASK         0xFFFFFF

/// interface index is formed from
///     t_ --> type of the interface (4 bits)
///     s_ --> physical slot of the card  (4 bits)
///     p_ --> parent port (1 based) (8 bits)
///     c_ --> child port (0 based) (16 bits)
/// based on the type of the interface, the slot bits will be
/// used for a running identifier of that type of interface
/// e.g. IF_TYPE_UPLINK or IF_TYPE_TUNNEL etc.
/// NOTE: child port 0 ==> non-broken port
#define IFINDEX(t_, s_, p_, c_)                          \
            ((t_ << IF_TYPE_SHIFT)            |          \
             ((s_) << ETH_IF_SLOT_SHIFT)      |          \
             (p_ << ETH_IF_PARENT_PORT_SHIFT) | (c_))

#define ETH_IFINDEX(s_, p_, c_)    IFINDEX(IF_TYPE_ETH, (s_), (p_), (c_))
#define MGMT_INTERNAL_IFINDEX(s_, p_, c_)                   \
            IFINDEX(IF_TYPE_MGMT_INTERNAL, (s_), (p_), (c_))
#define UPLINK_PC_IFINDEX(pc_id_)  ((IF_TYPE_UPLINK_PC << IF_TYPE_SHIFT) | \
                                    (pc_id_))
#define L3_IFINDEX(if_id_)         ((IF_TYPE_L3 << IF_TYPE_SHIFT) | (if_id_))
#define LIF_IFINDEX(if_id_)        ((uint32_t)(IF_TYPE_LIF << IF_TYPE_SHIFT) | (if_id_))
#define LOOPBACK_IFINDEX(if_id_)   ((IF_TYPE_LOOPBACK << IF_TYPE_SHIFT) | (if_id_))
#define CONTROL_IFINDEX(if_id_)    ((IF_TYPE_CONTROL << IF_TYPE_SHIFT) | (if_id_))
#define HOST_IFINDEX(if_id_)       ((IF_TYPE_HOST << IF_TYPE_SHIFT) | (if_id_))
#define DPA_IFINDEX(if_id_)        ((IF_TYPE_DPA << IF_TYPE_SHIFT) | (if_id_))
#define CRYPTO_IFINDEX(if_id_)     ((IF_TYPE_CRYPTO << IF_TYPE_SHIFT) | (if_id_))

#define IFINDEX_TO_IFTYPE(ifindex_)                      \
            ((ifindex_ >> IF_TYPE_SHIFT) & IF_TYPE_MASK)

#define IFINDEX_TO_IFID(ifindex_)                        \
            ((ifindex_) & ~(IF_TYPE_MASK << IF_TYPE_SHIFT))

#define ETH_IFINDEX_TO_SLOT(ifindex_)                    \
            ((ifindex_ >> ETH_IF_SLOT_SHIFT) & ETH_IF_SLOT_MASK)

#define ETH_IFINDEX_TO_PARENT_PORT(ifindex_)             \
            ((ifindex_ >> ETH_IF_PARENT_PORT_SHIFT) & ETH_IF_PARENT_PORT_MASK)

#define ETH_IFINDEX_TO_CHILD_PORT(ifindex_)              \
            (ifindex_ & ETH_IF_CHILD_PORT_MASK)

#define ETH_IFINDEX_TO_UPLINK_IFINDEX(ifindex_)          \
            ((IF_TYPE_UPLINK << IF_TYPE_SHIFT) |         \
             ((ifindex_) & ~(IF_TYPE_MASK << IF_TYPE_SHIFT)))

#define UPLINK_IFINDEX_TO_ETH_IFINDEX(ifindex_)          \
            ((IF_TYPE_ETH << IF_TYPE_SHIFT) |            \
             ((ifindex_) & ~(IF_TYPE_MASK << IF_TYPE_SHIFT)))

#define LIF_IFINDEX_TO_LIF_ID(ifindex_)                  \
            (ifindex_ & LIF_IF_LIF_ID_MASK)

#define LOOPBACK_IFINDEX_TO_LOOPBACK_IF_ID(ifindex_)     \
            (ifindex_ & LOOPBACK_IF_ID_MASK)

#define HOST_IFINDEX_TO_IF_ID(ifindex_)                  \
            (ifindex_ & HOST_IF_ID_MASK)

#define DPA_IFINDEX_TO_IF_ID(ifindex_)                   \
            (ifindex_ & DPA_IF_ID_MASK)

#define CRYPTO_IFINDEX_TO_IF_ID(ifindex_)                \
            (ifindex_ & CRYPTO_IF_ID_MASK)

/// \brief    given ifindex, compute the logical port
/// \param[in] ifindex    interface index
/// \return    logical port corresponding to the interface index
static inline uint32_t
ifindex_to_logical_port (if_index_t ifindex)
{
    uint32_t if_type = IFINDEX_TO_IFTYPE(ifindex);
    uint32_t parent = ETH_IFINDEX_TO_PARENT_PORT(ifindex);
    uint32_t child = ETH_IFINDEX_TO_CHILD_PORT(ifindex);

    if (if_type == IF_TYPE_MGMT_INTERNAL) {
        return  ((parent-1) * LINKMGR_MAX_PORT_LANES) + (child + 1);
    }

    if (child == 0) {
        return ((parent - 1) * LINKMGR_MAX_PORT_LANES) + child + 1;
    } else {
        return ((parent - 1) * LINKMGR_MAX_PORT_LANES) + child;
    }
}
/// \brief    given ifindex, compute the phy port
/// \param[in] ifindex    interface index
/// \return    phy port corresponding to the interface index
static inline int
ifindex_to_phy_port (if_index_t ifindex)
{
    int parent_port = ETH_IFINDEX_TO_PARENT_PORT(ifindex);

    // return -1 for mgmt port
    if (parent_port > MAX_PHY_PORT) {
        return -1;
    }
    return parent_port;
}

#ifdef __cplusplus
}
#endif

#endif    // __INCLUDE_IF_H__
