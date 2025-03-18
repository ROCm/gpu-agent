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
// base SDK header file that goes into rest of the SDK
//------------------------------------------------------------------------------

#ifndef __SDK_BASE_HPP__
#define __SDK_BASE_HPP__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <inttypes.h>
#if defined(__cplusplus) && !defined(RTOS)
#include <ostream>
#include <map>
#endif

#ifdef __cplusplus
namespace sdk {
#endif

#define TRUE                                         1
#define FALSE                                        0

#define SDK_MAX_NAME_LEN                             16

#define _In_
#define _Out_
#define _Inout_

#ifndef __PACK__
#define __PACK__            __attribute__((packed))
#endif

#define __UNUSED__          __attribute__((unused))

#define __ALIGN__(_n_)      __attribute__((__aligned__(_n_)))
#define __CACHE_ALIGN__     __ALIGN__(CACHE_LINE_SIZE)

#define SDK_MAX(a, b)                                      \
    ({ __typeof__ (a) _a = (a);                            \
       __typeof__ (b) _b = (b);                            \
       _a > _b ? _a : _b;                                  \
     })

#define SDK_MIN(a, b)                                      \
    ({ __typeof__ (a) _a = (a);                            \
       __typeof__ (b) _b = (b);                            \
       _a < _b ? _a : _b;                                  \
     })

//-----------------------------------------------------------------------------
// X-Macro for defining enums (generates enum definition and string formatter)
//
// Example:
//
// #define SAMPLE_ENUM_ENTRIES(ENTRY)
//    ENTRY(OK, 0, "ok")
//    ENTRY(ERR, 1, "err")
//
// SDK_DEFINE_ENUM(sample_enum_t, SAMPLE_ENUM_ENTRIES)
//------------------------------------------------------------------------------
#define _SDK_ENUM_FIELD(_name, _val, _desc) _name = _val,
#define _SDK_ENUM_STR(_name, _val, _desc)                               \
            inline const char *_name ## _str() {return _desc; }
#define _SDK_ENUM_CASE(_name, _val, _desc) case _name: return os << #_desc;
#define _SDK_ENUM_CASE_STR(_name, _val, _desc) case _name: return #_name;
#define _SDK_ENUM_CASE_DESC(_name, _val, _desc) case _name: return _desc;
#define _SDK_MAP_ENTRY(_name, _val, _desc) {_desc, _name},

#if defined(__cplusplus) && !defined(RTOS)

#if !defined(likely)
#define likely(x)           __builtin_expect(!!(x), 1)
#define unlikely(x)         __builtin_expect(!!(x), 0)
#endif

#define SDK_DEFINE_ENUM(_type, _entries)                                \
    typedef enum { _entries(_SDK_ENUM_FIELD) } _type;                   \
    inline std::ostream& operator<<(std::ostream& os, _type t) {        \
        switch (t) {                                                    \
            _entries(_SDK_ENUM_CASE);                                   \
        }                                                               \
        return os;                                                      \
    }                                                                   \
    _entries(_SDK_ENUM_STR)

#define SDK_DEFINE_MAP(_typ, _entries)                                  \
    std::map<std::string, _typ> _entries##_map =                        \
    {                                                                   \
        _entries(_SDK_MAP_ENTRY)                                        \
    };

#define SDK_DEFINE_MAP_EXTERN(_typ, _entries)                           \
    extern std::map<std::string, _typ> _entries##_map;

// --------------------------------------------
// Call to fetch dir used as temporary storage
// based on env var TMP_DIR. If env var not
// specified, return "/tmp" as default
// --------------------------------------------
static inline std::string
get_tmp_dir (void) {
    const char* tmp_dir;
    tmp_dir = std::getenv("TMP_DIR");
    if (tmp_dir != NULL) {
        return std::string(tmp_dir);
    } else {
        return "/tmp";
    }
}

#else

#define SDK_DEFINE_ENUM(_type, _entries)                                \
    typedef enum { _entries(_SDK_ENUM_FIELD) } _type;                   \
    _entries(_SDK_ENUM_STR)

#define SDK_DEFINE_MAP(_typ, _entries)
#define SDK_DEFINE_MAP_EXTERN(_typ, _entries)

#endif

#define SDK_DEFINE_ENUM_TO_STR(_typ, _entries)                          \
    inline const char *_entries##_str(_typ c)                           \
    {                                                                   \
        switch (c) {                                                    \
            _entries(_SDK_ENUM_CASE_STR);                               \
        }                                                               \
        return "unknown";                                               \
    }

#define SDK_DEFINE_ENUM_TO_DESC(_typ, _entries)                         \
    inline const char *_entries##_desc(_typ c)                          \
    {                                                                   \
        switch (c) {                                                    \
            _entries(_SDK_ENUM_CASE_DESC);                              \
        }                                                               \
        return "unknown";                                               \
    }

//------------------------------------------------------------------------------
// atomic operations
//------------------------------------------------------------------------------
#define SDK_ATOMIC_INC_UINT32(ptr, val)     __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define SDK_ATOMIC_DEC_UINT32(ptr, val)     __atomic_sub_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define SDK_ATOMIC_STORE_UINT32(ptr, vptr)  __atomic_store(ptr, vptr, __ATOMIC_SEQ_CST)
#define SDK_ATOMIC_LOAD_UINT32(ptr, vptr)   __atomic_load(ptr, vptr, __ATOMIC_SEQ_CST)
#define SDK_ATOMIC_EXCHANGE_UINT32(ptr, vptr, rptr) __atomic_exchange(ptr, vptr, rptr, __ATOMIC_SEQ_CST)

#define SDK_ATOMIC_INC_UINT64(ptr, val)     SDK_ATOMIC_INC_UINT32(ptr, val)
#define SDK_ATOMIC_DEC_UINT64(ptr, val)     SDK_ATOMIC_DEC_UINT32(ptr, val)
#define SDK_ATOMIC_STORE_UINT64(ptr, vptr)  SDK_ATOMIC_STORE_UINT32(ptr, vptr)
#define SDK_ATOMIC_LOAD_UINT64(ptr, vptr)   SDK_ATOMIC_LOAD_UINT32(ptr, vptr)
#define SDK_ATOMIC_EXCHANGE_UINT64(ptr, vptr, rptr) SDK_ATOMIC_EXCHANGE_UINT32(ptr, vptr, rptr)

#define SDK_ATOMIC_FETCH_ADD(ptr, val)      __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED)
#define SDK_ATOMIC_FETCH_SUB(ptr, val)      __atomic_fetch_sub(ptr, val, __ATOMIC_RELAXED)
#define SDK_ATOMIC_ADD_FETCH(ptr, val)      __atomic_add_fetch(ptr, val, __ATOMIC_RELAXED)
#define SDK_ATOMIC_SUB_FETCH(ptr, val)      __atomic_sub_fetch(ptr, val, __ATOMIC_RELAXED)

#define SDK_ATOMIC_LOAD_BOOL(ptr)           __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define SDK_ATOMIC_STORE_BOOL(ptr, val)     __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)

#define SDK_ATOMIC_LOAD_UINT16(ptr)         SDK_ATOMIC_LOAD_BOOL(ptr)

#define SDK_ATOMIC_COMPARE_EXCHANGE_WEAK(ptr, vptr, val) \
   __atomic_compare_exchange_n(ptr, vptr, val, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

#define SDK_ATOMIC_COMPARE_EXCHANGE(ptr, vptr, val) \
   __atomic_compare_exchange_n(ptr, vptr, val, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

// light weight lock and unlock macros
#define SDK_VOLATILE_LOCK(__lock_ptr__)                                      \
{                                                                            \
    uint8_t _lock_free_ = 0;                                                 \
                                                                             \
    while (!__atomic_compare_exchange_n((__lock_ptr__), &_lock_free_, 1, 0,  \
           __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {                            \
        while (__atomic_load_n((__lock_ptr__), __ATOMIC_RELAXED));           \
        /* on failure, compare_exchange writes (*p)->lock into free */       \
        _lock_free_ = 0;                                                     \
    }                                                                        \
}

#define SDK_VOLATILE_UNLOCK(__lock_ptr__)                                    \
            __atomic_store_n((__lock_ptr__), 0, __ATOMIC_RELEASE);

#define SDK_ARRAY_SIZE(arr)                (sizeof((arr))/sizeof((arr)[0]))

#define SDK_RET_OK                            ((uint32_t)0)
#define SDK_RET_OOM                           ((uint32_t)1)
#define SDK_RET_INVALID_ARG                   ((uint32_t)2)
#define SDK_RET_INVALID_OP                    ((uint32_t)3)
#define SDK_RET_ENTRY_NOT_FOUND               ((uint32_t)4)
#define SDK_RET_ENTRY_EXISTS                  ((uint32_t)5)
#define SDK_RET_NO_RESOURCE                   ((uint32_t)6)
#define SDK_RET_TABLE_FULL                    ((uint32_t)7)
#define SDK_RET_OOB                           ((uint32_t)8)
#define SDK_RET_HW_PROGRAM_ERR                ((uint32_t)9)
#define SDK_RET_RETRY                         ((uint32_t)10)
#define SDK_RET_NOOP                          ((uint32_t)11)
#define SDK_RET_DUPLICATE_FREE                ((uint32_t)12)
#define SDK_RET_COLLISION                     ((uint32_t)13)
#define SDK_RET_MAX_RECIRC_EXCEED             ((uint32_t)14)
#define SDK_RET_HW_READ_ERR                   ((uint32_t)15)
#define SDK_RET_TXN_NOT_FOUND                 ((uint32_t)16)
#define SDK_RET_TXN_EXISTS                    ((uint32_t)17)
#define SDK_RET_TXN_INCOMPLETE                ((uint32_t)18)
#define SDK_RET_COMM_FAIL                     ((uint32_t)19)
#define SDK_RET_HW_SW_OO_SYNC                 ((uint32_t)20)
#define SDK_RET_OBJ_CLONE_ERR                 ((uint32_t)21)
#define SDK_RET_IN_PROGRESS                   ((uint32_t)22)
#define SDK_RET_DEVICE_RESET_ERR              ((uint32_t)23)
#define SDK_RET_UPG_CC_DPDK_DEVICE_ERR        ((uint32_t)24)
#define SDK_RET_UPG_CC_IPXE_DEVICE_ERR        ((uint32_t)25)
#define SDK_RET_UPG_CC_NON_ETH_DEVICE_ERR     ((uint32_t)26)
#define SDK_RET_UPG_DPA_SYNC_INIT_ERR         ((uint32_t)27)
#define SDK_RET_UPG_DPA_SYNC_OBJ_ERR          ((uint32_t)28)
#define SDK_RET_UPG_BGP_SYNC_INIT_ERR         ((uint32_t)29)
#define SDK_RET_UPG_BGP_SYNC_OBJ_ERR          ((uint32_t)30)
#define SDK_RET_BGP_QUIESCE_START_ERR         ((uint32_t)31)
#define SDK_RET_BGP_QUIESCE_STOP_ERR          ((uint32_t)32)
#define SDK_RET_HW_QUIESCE_START_ERR          ((uint32_t)33)
#define SDK_RET_HW_QUIESCE_STOP_ERR           ((uint32_t)34)
#define SDK_RET_DEVICE_QUIESCE_START_ERR      ((uint32_t)35)
#define SDK_RET_DEVICE_QUIESCE_STOP_ERR       ((uint32_t)36)
#define SDK_RET_UPG_PSTATE_SYNC_ERR           ((uint32_t)37)
#define SDK_RET_RSC_MAPPING_CONFLICT          ((uint32_t)38)
#define SDK_RET_TIMEOUT                       ((uint32_t)39)
#define SDK_RET_ENTRY_READ_PARTIAL            ((uint32_t)40)
#define SDK_RET_ENTRY_BUSY                    ((uint32_t)41)
#define SDK_RET_HW_ENTRY_EXISTS               ((uint32_t)42)
#define SDK_RET_SVC_NOT_ACTIVE                ((uint32_t)43)
#define SDK_RET_OP_NOT_SUPPORTED              ((uint32_t)44)
#define SDK_RET_AGENT_NOT_ACTIVATED           ((uint32_t)45)
#define SDK_RET_IN_USE                        ((uint32_t)46)
#define SDK_RET_FSM                           ((uint32_t)47)
#define SDK_RET_FSM_SVC                       ((uint32_t)48)
#define SDK_RET_ERANGE                        ((uint32_t)49)
#define SDK_RET_OP_NOT_PROCESSED              ((uint32_t)50)
#define SDK_RET_FILE_ERR                      ((uint32_t)51)
#define SDK_RET_PERMISSION_ERR                ((uint32_t)52)
#define SDK_RET_INTERNAL_EXCEPTION_ERR        ((uint32_t)53)
#define SDK_RET_INIT_ERR                      ((uint32_t)54)
#define SDK_RET_INTERRUPT                     ((uint32_t)55)
#define SDK_RET_UNEXPECTED_DATA_SIZE_ERR      ((uint32_t)56)
#define SDK_RET_NO_DATA_ERR                   ((uint32_t)57)
#define SDK_RET_UNEXPECTED_DATA_ERR           ((uint32_t)58)
#define SDK_RET_REFCOUNT_OVERFLOW_ERR         ((uint32_t)59)
#define SDK_RET_SETTING_UNAVAILABLE_ERR       ((uint32_t)60)
#define SDK_RET_RESTART_ERR                   ((uint32_t)61)
#define SDK_RET_IPC_OPEN_ERR                  ((uint32_t)62)
#define SDK_RET_IPC_MSG_ERR                   ((uint32_t)63)
#define SDK_RET_IPC_SIZE_MISTMATCH            ((uint32_t)64)
#define SDK_RET_ERR                           ((uint32_t)255)
#define SDK_RET_MAX                           SDK_RET_ERR

#ifdef __cplusplus
struct sdk_ret_t {
public:
    // default constructor
    sdk_ret_t() {
        status_ = 0;
        err_code_ = 0;
    }
    // constructor
    sdk_ret_t(uint32_t status) : status_(status) {
        err_code_ = 0;
    }
    // constructor
    sdk_ret_t(uint32_t status, uint32_t err_code) :
                 status_(status), err_code_(err_code) {}
    // equality operator
    bool operator==(const sdk_ret_t& other) const {
        return (status_ == other.status_);
    }
    // inequality operator
    bool operator!=(const sdk_ret_t& other) const {
        return (status_ != other.status_);
    }
    // assignment operator
    void operator=(const uint32_t& status) { status_ = status; }
    // accessor for the status
    uint32_t operator()(void) const { return status_; }
    // set status and error code
    void set(uint32_t status, uint32_t err_code) {
        status_ = status;
        err_code_ = err_code;
    }
    // accessor for the error code
    uint32_t err_code(void) const { return err_code_; }
    // stringify the status code
    const char *str(void) const{
        switch (status_) {
        case SDK_RET_OK:
            return "ok";
        case SDK_RET_OOM:
            return "out of memory error";
        case SDK_RET_INVALID_ARG:
            return "invalid arg";
        case SDK_RET_INVALID_OP:
            return "invalid operation";
        case SDK_RET_ENTRY_NOT_FOUND:
            return "lookup failure";
        case SDK_RET_ENTRY_EXISTS:
            return "already present";
        case SDK_RET_NO_RESOURCE:
            return "resource exhaustion";
        case SDK_RET_TABLE_FULL:
            return "h/w table full";
        case SDK_RET_OOB:
            return "out-of-bound error";
        case SDK_RET_HW_PROGRAM_ERR:
            return "h/w programming error";
        case SDK_RET_RETRY:
            return "retry the operation";
        case SDK_RET_NOOP:
            return "no operation performed";
        case SDK_RET_DUPLICATE_FREE:
            return "duplicate free";
        case SDK_RET_COLLISION:
            return "collision detected";
        case SDK_RET_MAX_RECIRC_EXCEED:
            return "maximum recirc exceeded";
        case SDK_RET_HW_READ_ERR:
            return "h/w read error";
        case SDK_RET_TXN_NOT_FOUND:
            return "transaction not found";
        case SDK_RET_TXN_EXISTS:
            return "transaction exists";
        case SDK_RET_TXN_INCOMPLETE:
            return "transaction incomplete";
        case SDK_RET_COMM_FAIL:
            return "communication failure";
        case SDK_RET_HW_SW_OO_SYNC:
            return "h/w, s/w out of sync";
        case SDK_RET_OBJ_CLONE_ERR:
            return "object clone failure";
        case SDK_RET_IN_PROGRESS:
            return "operation in progress";
        case SDK_RET_DEVICE_RESET_ERR:
            return "device reset failed";
        case SDK_RET_UPG_CC_DPDK_DEVICE_ERR:
            return "dpdk device comapt check error";
        case SDK_RET_UPG_CC_IPXE_DEVICE_ERR:
            return "ipxe device compat check error";
        case SDK_RET_UPG_CC_NON_ETH_DEVICE_ERR:
            return "non-eth device compat check error" ;
        case SDK_RET_UPG_DPA_SYNC_INIT_ERR:
            return "datapath assist sync init failure";
        case SDK_RET_UPG_DPA_SYNC_OBJ_ERR:
            return "datapath assist objects sync failure";
        case SDK_RET_UPG_BGP_SYNC_INIT_ERR:
            return "BGP sync init failure";
        case SDK_RET_UPG_BGP_SYNC_OBJ_ERR:
            return "BGP objects sync failure";
        case SDK_RET_BGP_QUIESCE_START_ERR:
            return "BGP quiesce start failed";
        case SDK_RET_BGP_QUIESCE_STOP_ERR:
            return "BGP quiesce stop failed";
        case SDK_RET_HW_QUIESCE_START_ERR:
            return "hardware quiesce start failed";
        case SDK_RET_HW_QUIESCE_STOP_ERR:
            return "hardware quiesce stop failed";
        case SDK_RET_DEVICE_QUIESCE_START_ERR:
            return "device quiesce start failed";
        case SDK_RET_DEVICE_QUIESCE_STOP_ERR:
            return "device quiesce stop failed";
        case SDK_RET_UPG_PSTATE_SYNC_ERR:
            return "persistent state sync failed";
        case SDK_RET_RSC_MAPPING_CONFLICT:
            return "resource mapping conflict";
        case SDK_RET_TIMEOUT:
            return "operation timed out";
        case SDK_RET_ENTRY_READ_PARTIAL:
            return "partial entry is read";
        case SDK_RET_ENTRY_BUSY:
            return "entry is in-use/busy";
        case SDK_RET_HW_ENTRY_EXISTS:
            return "entry exists in h/w table";
        case SDK_RET_SVC_NOT_ACTIVE:
            return "service not running/active";
        case SDK_RET_OP_NOT_SUPPORTED:
            return "operation not supported";
        case SDK_RET_AGENT_NOT_ACTIVATED:
            return "agent not activated";
        case SDK_RET_IN_USE:
            return "in use";
        case SDK_RET_ERR:
            return "unspecified error";
        case SDK_RET_FSM:
            return "return from fsm orchestrator";
        case SDK_RET_FSM_SVC:
            return "in-progress fsm return from service";
        case SDK_RET_ERANGE:
            return "parameter out of range";
        case SDK_RET_OP_NOT_PROCESSED:
            return "operation not processed";
        case SDK_RET_FILE_ERR:
            return "problem accessing a file";
        case SDK_RET_PERMISSION_ERR:
            return "permission denied error";
        case SDK_RET_INTERNAL_EXCEPTION_ERR:
            return "internal exception";
        case SDK_RET_INIT_ERR:
            return "internal initializing error";
        case SDK_RET_INTERRUPT:
            return "interrupt occured";
        case SDK_RET_UNEXPECTED_DATA_SIZE_ERR:
            return "unexpected size of data read";
        case SDK_RET_NO_DATA_ERR:
            return "no data found";
        case SDK_RET_UNEXPECTED_DATA_ERR:
            return "unexpected data read";
        case SDK_RET_REFCOUNT_OVERFLOW_ERR:
            return "ref counter exceeded";
        case SDK_RET_SETTING_UNAVAILABLE_ERR:
            return "setting unavialable";
        case SDK_RET_RESTART_ERR:
            return "failed to restart driver";
        default:
            return "unknown error";
        }
    }

private:
    // status code
    uint32_t status_;
    // err code is specific to status code above
    // and is only applicable in cases where status_
    // is not SDK_RET_OK
    uint32_t err_code_;
};
#else    // __cplusplus
typedef uint32_t sdk_ret_t;
#endif   // __cplusplus

typedef enum sdk_status_e {
    SDK_STATUS_NONE,
    SDK_STATUS_ASIC_INIT_DONE,
    SDK_STATUS_MEM_INIT_DONE,
    SDK_STATUS_PACKET_BUFFER_INIT_DONE,
    SDK_STATUS_DATA_PLANE_INIT_DONE,
    SDK_STATUS_SCHEDULER_INIT_DONE,
    SDK_STATUS_ASIC_CSR_INIT_DONE,
    SDK_STATUS_UP
} sdk_status_t;
#define INVALID_INDEXER_INDEX                        0xFFFFFFFF

#ifndef RTOS
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#define SDK_MEMORY_TRIM()        malloc_trim(0)

#define SET_BIT(p,n)   ((p) |=   (1 << (n)))
#define CLR_BIT(p,n)   ((p) &=  ~(1 << (n)))
#define FLIP_BIT(p,n)  ((p) ^=   (1 << (n)))
#define CHECK_BIT(p,n) ((p) &    (1 << (n)))

#ifdef __cplusplus
}    // namespace sdk

using sdk::sdk_ret_t;
using sdk::sdk_status_t;
#endif   // __cplusplus

static inline void
to_hexstr (const uint8_t *ba, int len, char *str) {
    int i = 0;
    for (i = 0; i < len; i++) {
        snprintf(str + i * 3, (len * 3) - (3 * i), "%02x ", ba[i]);
    }
    str[i * 3] = 0;
}

#endif    // __SDK_BASE_HPP__

