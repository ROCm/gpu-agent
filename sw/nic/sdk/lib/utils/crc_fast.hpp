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


#ifndef __CRC_FAST_HPP__
#define __CRC_FAST_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"

namespace sdk {
namespace utils {

typedef enum crc15_polynomial_type_e {
    CRC15_POLYNOMIAL_TYPE_WAY0      = 0,
    CRC15_POLYNOMIAL_TYPE_WAY1      = 1,
    CRC15_POLYNOMIAL_TYPE_WAY2      = 2,
    CRC15_POLYNOMIAL_TYPE_WAY3      = 3,
    CRC15_POLYNOMIAL_TYPE_MAX,
} crc15_polynomial_type_t;

typedef enum crc16_polynomial_type_e {
    CRC16_POLYNOMIAL_TYPE_WAY0      = 0,
    CRC16_POLYNOMIAL_TYPE_WAY1      = 1,
    CRC16_POLYNOMIAL_TYPE_WAY2      = 2,
    CRC16_POLYNOMIAL_TYPE_WAY3      = 3,
    CRC16_POLYNOMIAL_TYPE_MAX,
} crc16_polynomial_type_t;

typedef enum crc32_polynomial_type_e {
    CRC32_POLYNOMIAL_TYPE_CRC32     = 0,
    CRC32_POLYNOMIAL_TYPE_CRC32C    = 1,
    CRC32_POLYNOMIAL_TYPE_CRC32K    = 2,
    CRC32_POLYNOMIAL_TYPE_CRC32Q    = 3,
    CRC32_POLYNOMIAL_TYPE_MAX,
} crc32_polynomial_type_t;

extern uint16_t crc15(uint8_t const message[], int nBytes, uint8_t poly_index);
extern uint16_t crc16(uint8_t const message[], int nBytes, uint8_t poly_index);
extern uint32_t crc32(uint8_t const message[], int nBytes, uint8_t poly_index);

}   // namespace utils
}   // namespace sdk
#endif    //__CRC_FAST_HPP__
