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


#include "crc_fast.hpp"
#include "include/sdk/mem.hpp"
#include "include/sdk/assert.hpp"

namespace sdk {
namespace utils {

#define WIDTH_15        15
#define TOPBIT_15       (1 << (WIDTH_15 - 1))
#define WIDTH(x)        (8 * sizeof(x))
#define TOPBIT(x)       (1 << (WIDTH(x) - 1))

class crcFast {
public:
    uint16_t compute_crc15(uint8_t const message[], int nBytes,
                           uint8_t poly_index);
    uint16_t compute_crc16(uint8_t const message[], int nBytes,
                           uint8_t poly_index);
    uint32_t compute_crc32(uint8_t const message[], int nBytes,
                           uint8_t poly_index);
    crcFast();
private:
    uint16_t **crc15Table_;
    uint16_t **crc16Table_;
    uint32_t **crc32Table_;
    sdk_ret_t init_(void);
    sdk_ret_t init_crc15_poly_(uint8_t poly_index, uint16_t poly);
    sdk_ret_t init_crc16_poly_(uint8_t poly_index, uint16_t poly);
    sdk_ret_t init_crc32_poly_(uint8_t poly_index, uint32_t poly);
};

static uint16_t g_crc15_polynomials[] = {
    [CRC15_POLYNOMIAL_TYPE_WAY0]   = 0x80CF,
    [CRC15_POLYNOMIAL_TYPE_WAY1]   = 0x8437,
    [CRC15_POLYNOMIAL_TYPE_WAY2]   = 0x88C7,
    [CRC15_POLYNOMIAL_TYPE_WAY3]   = 0x99D5,
};

static uint16_t g_crc16_polynomials[] = {
    [CRC16_POLYNOMIAL_TYPE_WAY0]   = 0x80CF,
    [CRC16_POLYNOMIAL_TYPE_WAY1]   = 0x84C7,
    [CRC16_POLYNOMIAL_TYPE_WAY2]   = 0x88C7,
    [CRC16_POLYNOMIAL_TYPE_WAY3]   = 0x99D5,
};

static uint32_t g_crc32_polynomials[] = {
    [CRC32_POLYNOMIAL_TYPE_CRC32]  = 0x04C11DB7,
    [CRC32_POLYNOMIAL_TYPE_CRC32C] = 0x1EDC6F41,
    [CRC32_POLYNOMIAL_TYPE_CRC32K] = 0x741B8CD7,
    [CRC32_POLYNOMIAL_TYPE_CRC32Q] = 0x814141AB,
};

static crcFast g_crcFast;

//
// Wrapper around the global object compute method
//
uint16_t
crc15 (uint8_t const message[], int nBytes, uint8_t poly_index)
{
    return g_crcFast.compute_crc15(message, nBytes, poly_index);
}

uint16_t
crc16 (uint8_t const message[], int nBytes, uint8_t poly_index)
{
    return g_crcFast.compute_crc16(message, nBytes, poly_index);
}

uint32_t
crc32 (uint8_t const message[], int nBytes, uint8_t poly_index)
{
    return g_crcFast.compute_crc32(message, nBytes, poly_index);
}

//------------------------------------------------------------------------------
// constructor populates the tables
//------------------------------------------------------------------------------
crcFast::crcFast() {
    // allocate memory and init crc15 tables
    crc15Table_ = (uint16_t **)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                          CRC15_POLYNOMIAL_TYPE_MAX *
                                          sizeof(uint16_t *));
    assert(crc15Table_ != NULL);
    for (int i = 0; i < CRC15_POLYNOMIAL_TYPE_MAX; i++) {
        crc15Table_[i] = (uint16_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                                256 * sizeof(uint16_t));
        assert(crc15Table_[i] != NULL);
        init_crc15_poly_(i, g_crc15_polynomials[i]);
    }

    // allocate memory and init crc16 tables
    crc16Table_ = (uint16_t **)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                          CRC16_POLYNOMIAL_TYPE_MAX *
                                          sizeof(uint16_t *));
    assert(crc16Table_ != NULL);
    for (int i = 0; i < CRC16_POLYNOMIAL_TYPE_MAX; i++) {
        crc16Table_[i] = (uint16_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                                256 * sizeof(uint16_t));
        assert(crc16Table_[i] != NULL);
        init_crc16_poly_(i, g_crc16_polynomials[i]);
    }

    // allocate memory and init crc32 tables
    crc32Table_ = (uint32_t **)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                          CRC32_POLYNOMIAL_TYPE_MAX *
                                          sizeof(uint32_t *));
    assert(crc32Table_ != NULL);
    for (int i = 0; i < CRC32_POLYNOMIAL_TYPE_MAX; i++) {
        crc32Table_[i] = (uint32_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_CRCFAST,
                                                256 * sizeof(uint32_t));
        assert(crc32Table_[i] != NULL);
        init_crc32_poly_(i, g_crc32_polynomials[i]);
    }
}

//------------------------------------------------------------------------------
// initialize the crc15 table for the polynomial
//------------------------------------------------------------------------------
sdk_ret_t
crcFast::init_crc15_poly_(uint8_t poly_index, uint16_t poly)
{
    uint16_t remainder;

    // compute the remainder of each possible dividend
    for (int dividend = 0; dividend < 256; ++dividend) {
        // start with the dividend followed by zeros
        remainder = dividend << (WIDTH_15 - 8);

        // perform modulo-2 division, a bit at a time
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            // try to divide the current data bit
            if (remainder & TOPBIT_15) {
                remainder = (remainder << 1) ^ poly;
            }
            else {
                remainder = (remainder << 1);
            }
        }

        // store the result into the table
        crc15Table_[poly_index][dividend] = remainder;
    }

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// initialize the crc16 table for the polynomial
//------------------------------------------------------------------------------
sdk_ret_t
crcFast::init_crc16_poly_(uint8_t poly_index, uint16_t poly)
{
    uint16_t remainder;

    // compute the remainder of each possible dividend
    for (int dividend = 0; dividend < 256; ++dividend) {
        // start with the dividend followed by zeros
        remainder = dividend << (WIDTH(uint16_t) - 8);

        // perform modulo-2 division, a bit at a time
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            // try to divide the current data bit
            if (remainder & TOPBIT(uint16_t)) {
                remainder = (remainder << 1) ^ poly;
            }
            else {
                remainder = (remainder << 1);
            }
        }

        // store the result into the table
        crc16Table_[poly_index][dividend] = remainder;
    }

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// initialize the crc32 table for the polynomial
//------------------------------------------------------------------------------
sdk_ret_t
crcFast::init_crc32_poly_(uint8_t poly_index, uint32_t poly)
{
    uint32_t remainder;

    // compute the remainder of each possible dividend
    for (int dividend = 0; dividend < 256; ++dividend) {
        // start with the dividend followed by zeros
        remainder = dividend << (WIDTH(uint32_t) - 8);

        // perform modulo-2 division, a bit at a time
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            // try to divide the current data bit
            if (remainder & TOPBIT(uint32_t)) {
                remainder = (remainder << 1) ^ poly;
            }
            else {
                remainder = (remainder << 1);
            }
        }

        // store the result into the table
        crc32Table_[poly_index][dividend] = remainder;
    }

    return SDK_RET_OK;
}

uint16_t
crcFast::compute_crc15(uint8_t const message[], int nBytes, uint8_t poly_index)
{
    uint8_t data;
    uint16_t remainder = 0;

    SDK_ASSERT(poly_index < CRC16_POLYNOMIAL_TYPE_MAX);

    // divide the message by the polynomial, a byte at a time
    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH_15 - 8));
        remainder = crc15Table_[poly_index][data] ^ (remainder << 8);
    }

    // the final remainder is the CRC
    return (remainder & 0x7FFF);

}

uint16_t
crcFast::compute_crc16(uint8_t const message[], int nBytes, uint8_t poly_index)
{
    uint8_t data;
    uint16_t remainder = 0;

    SDK_ASSERT(poly_index < CRC16_POLYNOMIAL_TYPE_MAX);

    // divide the message by the polynomial, a byte at a time
    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH(uint16_t) - 8));
        remainder = crc16Table_[poly_index][data] ^ (remainder << 8);
    }

    // the final remainder is the CRC
    return (remainder);

}

uint32_t
crcFast::compute_crc32(uint8_t const message[], int nBytes, uint8_t poly_index)
{
    uint8_t data;
    uint32_t remainder = 0;

    SDK_ASSERT(poly_index < CRC32_POLYNOMIAL_TYPE_MAX);

    // divide the message by the polynomial, a byte at a time
    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH(uint32_t) - 8));
        remainder = crc32Table_[poly_index][data] ^ (remainder << 8);
    }

    // the final remainder is the CRC
    return (remainder);

}

}   // namespace utils
}   // namespace sdk
