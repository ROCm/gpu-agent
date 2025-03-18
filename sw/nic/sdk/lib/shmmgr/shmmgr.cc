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


#include <assert.h>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include "include/sdk/base.hpp"
#include "include/sdk/assert.hpp"
#include "include/sdk/mem.hpp"
#include "lib/shmmgr/shmmgr.hpp"

using namespace boost::interprocess;
#define TO_FM_SHM(x)       ((fixed_managed_shared_memory *)(x))
#define TO_SHM(x)          ((managed_shared_memory *)(x))
#define TO_FILE_MAP_SHM(x) ((managed_mapped_file *)(x))
#define TO_EXT_BUF_SHM(x)  ((managed_external_buffer *)(x))

namespace sdk {
namespace lib {

// base structure for named segment allocation
class shm_segment {
public:
    // offset of the allocated memory from the named segment
    uint64_t offset;
    // size of the allocated memory
    std::size_t size;
};

#define SHMMGR_OP(op, res) {                                        \
    if (mapped_file_) {                                             \
        SDK_ASSERT(fixed_ == false);                                \
        res = TO_FILE_MAP_SHM(mmgr_)->op;                           \
    } else if (ext_buff_) {                                         \
        res = TO_EXT_BUF_SHM(mmgr_)->op;                            \
    } else {                                                        \
        res = fixed_ ? TO_FM_SHM(mmgr_)->op : TO_SHM(mmgr_)->op;    \
    }                                                               \
}

#define SHMMGR_OP_NORET(op) {                                       \
    if (mapped_file_) {                                             \
        SDK_ASSERT(fixed_ == false);                                \
        TO_FILE_MAP_SHM(mmgr_)->op;                                 \
    } else if (ext_buff_) {                                         \
        TO_EXT_BUF_SHM(mmgr_)->op;                                  \
    } else {                                                        \
        fixed_ ? TO_FM_SHM(mmgr_)->op : TO_SHM(mmgr_)->op;          \
    }                                                               \
}

//------------------------------------------------------------------------------
// initialization routine
//------------------------------------------------------------------------------
bool
shmmgr::init(const char *name, const std::size_t size,
             shm_mode_e mode, void *baseaddr)
{
    if (baseaddr) {
        if (strchr(name, '/') == NULL) {
            fixed_managed_shared_memory    *fixed_mgr_shm;

            if (mode == SHM_CREATE_ONLY) {
                fixed_mgr_shm = new fixed_managed_shared_memory(create_only, name,
                                                                size, baseaddr);
            } else if (mode == SHM_OPEN_ONLY) {
                fixed_mgr_shm = new fixed_managed_shared_memory(open_only, name,
                                                                baseaddr);
            } else if (mode == SHM_OPEN_OR_CREATE) {
                fixed_mgr_shm = new fixed_managed_shared_memory(open_or_create, name,
                                                                size, baseaddr);
            } else if (mode == SHM_OPEN_READ_ONLY) {
                fixed_mgr_shm = new fixed_managed_shared_memory(open_read_only,
                                                                name, baseaddr);
            } else {
                return false;
            }
            mmgr_ = fixed_mgr_shm;
            mapped_file_ = false;
            SDK_TRACE_DEBUG("SHM manager init, fixed managed %s", name);
        } else {
            // TODO
            SDK_ASSERT(0);
        }
        fixed_ = true;
    } else {
        if (strchr(name, '/') == NULL) {
            managed_shared_memory    *mgr_shm;

            if (mode == SHM_CREATE_ONLY) {
                mgr_shm = new managed_shared_memory(create_only, name, size);
            } else if (mode == SHM_OPEN_ONLY) {
                mgr_shm = new managed_shared_memory(open_only, name);
            } else if (mode == SHM_OPEN_OR_CREATE) {
                mgr_shm = new managed_shared_memory(open_or_create, name, size);
            } else if (mode == SHM_OPEN_READ_ONLY) {
                mgr_shm = new managed_shared_memory(open_read_only, name);
            } else {
                return false;
            }
            mmgr_ = mgr_shm;
            mapped_file_ = false;
            SDK_TRACE_DEBUG("SHM manager init, managed %s", name);
        } else {
            managed_mapped_file    *mgr_map_file;

            if (mode == SHM_CREATE_ONLY) {
                mgr_map_file = new managed_mapped_file(create_only, name, size);
            } else if (mode == SHM_OPEN_ONLY) {
                mgr_map_file = new managed_mapped_file(open_only, name);
            } else if (mode == SHM_OPEN_OR_CREATE) {
                mgr_map_file = new managed_mapped_file(open_or_create, name, size);
            } else if (mode == SHM_OPEN_READ_ONLY) {
                mgr_map_file = new managed_mapped_file(open_read_only, name);
            } else {
                return false;
            }
            mmgr_ = mgr_map_file;
            mapped_file_ = true;
            SDK_TRACE_DEBUG("SHM manager init, file mapped %s", name);
        }
        fixed_ = false;
    }
    strncpy(name_, name, SHMSEG_NAME_MAX_LEN);
    name_[SHMSEG_NAME_MAX_LEN] = '\0';
    return true;
}

//------------------------------------------------------------------------------
// initialization method for a external buffer based shared memory implementation
//------------------------------------------------------------------------------
bool
shmmgr::ext_buff_init_(const std::size_t size, shm_mode_e mode, void *baseaddr) {
    if (mode == SHM_CREATE_ONLY) {
        mmgr_ = new managed_external_buffer(create_only, baseaddr, size);
    } else if ((mode == SHM_OPEN_ONLY) || (mode == SHM_OPEN_READ_ONLY)) {
        // TODO : read only support
        mmgr_ = new managed_external_buffer(open_only, baseaddr, size);
    } else {
        SDK_TRACE_ERR("Invalid mode %u is given", mode);
        return false;
    }
    ext_buff_ = true;
    name_[0] = '\0';
    SDK_TRACE_DEBUG("SHM manager init, external buff baseaddr 0x%lx, size %lu",
                    (long)baseaddr, size);
    return true;
}

//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
shmmgr::shmmgr()
{
    mmgr_ = NULL;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
shmmgr::~shmmgr()
{
}

//------------------------------------------------------------------------------
// factory method for name/address based shared memory
//------------------------------------------------------------------------------
shmmgr *
shmmgr::factory(const char *name, const std::size_t size,
                shm_mode_e mode, void *baseaddr, bool ext_buff)
{
    void      *mem;
    shmmgr    *new_shmmgr;

    // basic validation(s)
    if (name == NULL) {
        if (!ext_buff) {
            return NULL;
        }
    }
    if (size <= 16 && (mode == SHM_CREATE_ONLY || mode == SHM_OPEN_OR_CREATE)) {
        return NULL;
    }

    mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_SHM, sizeof(shmmgr));
    if (mem == NULL) {
        return NULL;
    }

    new_shmmgr = new (mem) shmmgr();
    if (ext_buff) {
        if (new_shmmgr->ext_buff_init_(size, mode, baseaddr) == false) {
            goto err_exit;
        }
    } else if (new_shmmgr->init(name, size, mode, baseaddr) == false) {
        goto err_exit;
    }
    return new_shmmgr;

err_exit:
    new_shmmgr->~shmmgr();
    SDK_FREE(SDK_MEM_ALLOC_LIB_SHM, new_shmmgr);
    return NULL;
}

//------------------------------------------------------------------------------
// remove shared memory given its name
//------------------------------------------------------------------------------
void
shmmgr::remove(const char *name)
{
    if (!name) {
        return;
    }
    SDK_TRACE_DEBUG("Deleting shared memory %s", name);
    if (strchr(name, '/') == NULL) {
        shared_memory_object::remove(name);
    } else {
        file_mapping::remove(name);
    }
}

//------------------------------------------------------------------------------
// destroy method
//------------------------------------------------------------------------------
void
shmmgr::destroy(shmmgr *mmgr)
{
    if (!mmgr) {
        return;
    }
    if (!mmgr->ext_buff_) {
        shmmgr::remove(mmgr->name_);
    }
    mmgr->~shmmgr();
    SDK_FREE(SDK_MEM_ALLOC_LIB_SHM, mmgr);
}

//------------------------------------------------------------------------------
// check if shared memory exists for the given name
//------------------------------------------------------------------------------
bool
shmmgr::exists(const char *name, void *baseaddr)
{
    if (name == NULL) {
        return false;
    }
    try {
        if (baseaddr) {
            fixed_managed_shared_memory seg(open_only, name, baseaddr);
        } else {
            if (strchr(name, '/') == NULL) {
                managed_shared_memory seg(open_only, name);
            } else {
                managed_mapped_file seg(open_only, name);
            }
        }
        //return segment.check_sanity();
        return true;
    } catch (const boost::interprocess::interprocess_exception &ex) {
        SDK_TRACE_VERBOSE("bip exception, %s",  ex.what());
    } catch (const std::exception &ex) {
        SDK_TRACE_VERBOSE("general exception, %s",  ex.what());
    }
    return false;
}

//------------------------------------------------------------------------------
// allocate requested amount of memory
//------------------------------------------------------------------------------
void *
shmmgr::alloc(const std::size_t size, const std::size_t alignment,
              bool reset)
{
    void    *ptr = NULL;

    if (alignment == 0) {
        SHMMGR_OP(allocate(size), ptr);
    } else {
        assert((alignment & (alignment - 1)) == 0);
        assert(alignment >= 4);
        SHMMGR_OP(allocate_aligned(size, alignment), ptr);
    }
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

//------------------------------------------------------------------------------
// free given memory
//------------------------------------------------------------------------------
void
shmmgr::free(void *mem)
{
    if (mem == NULL) {
        return;
    }
    SHMMGR_OP_NORET(deallocate(mem));
}

//------------------------------------------------------------------------------
// return the size of the shared memory
//------------------------------------------------------------------------------
std::size_t
shmmgr::size(void) const
{
    std::size_t size;

    SHMMGR_OP(get_size(), size);
    return size;
}

//------------------------------------------------------------------------------
// return the size of the free memory available
//------------------------------------------------------------------------------
std::size_t
shmmgr::free_size(void) const
{
    std::size_t size;

    SHMMGR_OP(get_free_memory(), size);
    return size;
}

void *
shmmgr::mmgr(void) const
{
    return mmgr_;
}

//------------------------------------------------------------------------------
// create/open a segment in the shared memory
//------------------------------------------------------------------------------
void *
shmmgr::segment_find(const char *name, bool create, std::size_t size,
                     std::size_t alignment) {
    std::pair<shm_segment*, std::size_t> res;
    shm_segment* state;
    void *addr = NULL;

    // TODO : if required provide the support
    SDK_ASSERT(fixed_ == false);

    SHMMGR_OP(find<shm_segment>(name), res);
    state = res.first;
    if (create) {
        // size should be given
        if (!size) {
            return NULL;
        }
        // free the exiting one
        if (state != NULL) {
            addr = (void *)((uint64_t)state - state->offset);
            SHMMGR_OP_NORET(deallocate(addr));
            SHMMGR_OP_NORET(destroy_ptr(state));
        }
        SHMMGR_OP(construct<shm_segment>(name)(), state);
        if (state) {
            try {
                if (alignment) {
                    assert((alignment & (alignment - 1)) == 0);
                    assert(alignment >= 4);
                    SHMMGR_OP(allocate_aligned(size, alignment), addr);
                } else {
                    SHMMGR_OP(allocate(size), addr);
                }
                if (addr) {
                    memset(state, 0, sizeof(shm_segment));
                    state->offset = (uint64_t)state - (uint64_t)addr;
                    state->size = size;
                    return addr;
                } else {
                    SDK_TRACE_ERR("Failed to allocate memory for segment %s",
                                  name);
                    SHMMGR_OP_NORET(destroy_ptr(state));
                }
            } catch (...) {
                SDK_TRACE_ERR("Failed to allocate memory for segment %s",
                              name);
                addr = NULL;
            }
        }
    } else {
        if (!state) {
            return NULL;
        }
        if (size && (state->size < size)) {
            SDK_TRACE_ERR("Failed to find segment %s, allocated size %lu "
                          "< requested size %lu", name, state->size, size);
        } else {
            addr = (void *)((uint64_t)state - state->offset);
        }
    }
    return addr;
}

std::size_t
shmmgr::get_segment_size(const char *name) const {
    std::pair<shm_segment*, std::size_t> res;
    shm_segment* state;

    SHMMGR_OP(find<shm_segment>(name), res);
    state = res.first;
    if (!state) {
        return 0;
    }
    return state->size;
}

bool
shmmgr::segment_exists(const char *name) const {
    std::pair<shm_segment*, std::size_t> res;

    // TODO : if required provide the support
    SDK_ASSERT(fixed_ == false);

    SHMMGR_OP(find<shm_segment>(name), res);
    if (res.first) {
        return true;
    } else {
        return false;
    }
}

bool
shmmgr::segment_free(const char *name) {
    std::pair<shm_segment*, std::size_t> res;
    shm_segment* state;
    void *addr;

    // TODO : if required provide the support
    SDK_ASSERT(fixed_ == false);

    SHMMGR_OP(find<shm_segment>(name), res);
    state = res.first;
    if (state) {
        addr = (void *)((uint64_t)state - state->offset);
        SHMMGR_OP_NORET(deallocate(addr));
        SHMMGR_OP_NORET(destroy_ptr(state));
        SDK_TRACE_DEBUG("Segment free is done %s", name);
        return true;
    } else {
        return false;
    }
}

template <typename T>
void shmmgr::iterate(T *mmgr, void *ctxt, shmmgr_seg_walk_cb_t cb) {
    typename T::const_named_iterator curr, it;
    typename T::const_named_iterator end;

    for (it = mmgr->named_begin(), end = mmgr->named_end();
        it != end;) {
        const shm_segment *seg = (const shm_segment*)it->value();
        void *addr = (void *)((uint64_t)seg - seg->offset);
        curr = it;
        ++it;
        cb(ctxt, curr->name(), seg->size, addr);
    };
};


void
shmmgr::segment_walk(void *ctxt, shmmgr_seg_walk_cb_t cb) {

    if (mapped_file_) {
        iterate<managed_mapped_file>(TO_FILE_MAP_SHM(mmgr_), ctxt, cb);
    } else if (ext_buff_) {
        iterate<managed_external_buffer>(TO_EXT_BUF_SHM(mmgr_), ctxt, cb);
    } else {
        if (fixed_) {
            iterate<fixed_managed_shared_memory>(TO_FM_SHM(mmgr_), ctxt, cb);
        } else {
            iterate<managed_shared_memory>(TO_SHM(mmgr_), ctxt, cb);
        }
    }
}

}    // namespace lib
}    // namespace sdk
