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


///----------------------------------------------------------------------------
///
/// \file
/// common interface APIs & macros
///
///----------------------------------------------------------------------------

#ifndef __IF_HPP__
#define __IF_HPP__

#include <string>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include "rtos-shared/include/if.h"

static inline std::string
ifindex_to_type_str (uint32_t ifindex)
{
    uint32_t type = IFINDEX_TO_IFTYPE(ifindex);

    switch (type) {
    case IF_TYPE_ETH:
        return "eth";
    case IF_TYPE_ETH_PC:
        return "eth-pc";
    case IF_TYPE_TUNNEL:
        return "tunnel";
    case IF_TYPE_UPLINK:
        return "uplink";
    case IF_TYPE_UPLINK_PC:
        return "uplink-pc";
    case IF_TYPE_L3:
        return "l3";
    case IF_TYPE_LIF:
        return "lif";
    case IF_TYPE_LOOPBACK:
        return "loopback";
    case IF_TYPE_CONTROL:
        return "control";
    case IF_TYPE_HOST:
        return "host";
    case IF_TYPE_MGMT_INTERNAL:
        return "internal-mgmt";
    case IF_TYPE_DPA:
        return "dpa";
    case IF_TYPE_CRYPTO:
        return "crypto";
    case IF_TYPE_NONE:
    default:
        return "none";
    }
}

static inline std::string
eth_ifindex_to_ifid_str (uint32_t ifindex, std::string sep="/")
{
    uint32_t slot, parent_port, child_port;

    slot = ETH_IFINDEX_TO_SLOT(ifindex);
    parent_port = ETH_IFINDEX_TO_PARENT_PORT(ifindex);
    child_port = ETH_IFINDEX_TO_CHILD_PORT(ifindex);
    if (child_port == 0) {
        return std::to_string(slot) + sep + std::to_string(parent_port);
    } else {
        return std::to_string(slot) + sep + std::to_string(parent_port) + sep +
            std::to_string(child_port);
    }
}

static inline std::string
eth_ifindex_to_str (uint32_t ifindex)
{
    if (ifindex == IFINDEX_INVALID) {
        return "-";
    }
    return ifindex_to_type_str(ifindex) + eth_ifindex_to_ifid_str(ifindex);
}

static inline std::string
eth_ifindex_to_str (uint32_t ifindex, std::string pfx)
{
    if (ifindex == IFINDEX_INVALID) {
        return "-";
    }
    return pfx + ifindex_to_type_str(ifindex) +
               eth_ifindex_to_ifid_str(ifindex);
}

static inline uint32_t
eth_str_to_ifindex (std::string str, std::string sep = "/")
{
    int slot;
    int child_port;
    int parent_port;
    unsigned int index;

    std::transform(str.begin(), str.end(),str.begin(), ::toupper);
    if (str.compare(0, 3, "ETH") != 0) {
        return IFINDEX_INVALID;
    }
    index = 3;
    if (index >= str.length() || !isdigit(str[index])) {
        return IFINDEX_INVALID;
    }

    slot = str[index] - '0';
    index++;

    if (index >= str.length()) {
        return IFINDEX_INVALID;
    }

    if (str.compare(index, sep.length(), sep) != 0) {
        return IFINDEX_INVALID;
    }
    index += sep.length();

    if (index >= str.length() || !isdigit(str[index])) {
        return IFINDEX_INVALID;
    }
    parent_port = str[index] - '0';
    index++;

    if (index == str.length()) {
        return ETH_IFINDEX(slot, parent_port, 0);
    }

    if (str.compare(index, sep.length(), sep) != 0) {
        return IFINDEX_INVALID;
    }
    index += sep.length();

    if (index >= str.length() || !isdigit(str[index])) {
        return IFINDEX_INVALID;
    }
    child_port = str[index] - '0';
    index++;

    if (index != str.length()) {
        return IFINDEX_INVALID;
    }
    return ETH_IFINDEX(slot, parent_port, child_port);
}

static inline if_type_t
ifindex_to_if_type (if_index_t ifindex)
{
    switch (IFINDEX_TO_IFTYPE(ifindex)) {
    case IF_TYPE_ETH:
        return IF_TYPE_ETH;
    case IF_TYPE_UPLINK:
        return IF_TYPE_UPLINK;
    case IF_TYPE_UPLINK_PC:
        return IF_TYPE_UPLINK_PC;
    case IF_TYPE_L3:
        return IF_TYPE_L3;
    case IF_TYPE_LOOPBACK:
        return IF_TYPE_LOOPBACK;
    default:
        break;
    }
    return IF_TYPE_NONE;
}

static inline std::string
if_type_to_str (if_type_t iftype)
{
    switch (iftype) {
    case IF_TYPE_ETH:
    case IF_TYPE_UPLINK:
        return "uplink";
    case IF_TYPE_UPLINK_PC:
        return "uplink-pc";
    case IF_TYPE_L3:
        return "l3";
    case IF_TYPE_LOOPBACK:
        return "lo";
    case IF_TYPE_HOST:
        return "host";
    case IF_TYPE_DPA:
        return "dpa";
    case IF_TYPE_CRYPTO:
        return "crypto";
    default:
        break;
    }
    return "none";
}

#endif    // __IF_HPP__
