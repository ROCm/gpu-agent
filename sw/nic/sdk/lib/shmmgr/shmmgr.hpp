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


#ifndef __SDK_SHMMGR_HPP__
#define __SDK_SHMMGR_HPP__

namespace sdk {
namespace lib {

//------------------------------------------------------------------------------
// modes in which a shared memory segment can be opened/created
//------------------------------------------------------------------------------
enum shm_mode_e {
    SHM_CREATE_ONLY,
    SHM_OPEN_ONLY,
    SHM_OPEN_OR_CREATE,
    SHM_OPEN_READ_ONLY,
};

#define SHMSEG_NAME_MAX_LEN    256

// segment walk callback function
typedef void (*shmmgr_seg_walk_cb_t)(void *ctx, const char *name,
                                     std::size_t size, void *addr);

//------------------------------------------------------------------------------
// shmmgr is the shared memory and it manages all the shared memory for
// interprocess management
//------------------------------------------------------------------------------
class shmmgr {
public:
    /// \brief create the shared memory manager
    /// \param[in] name  name of the shared memory segment being created
    /// \param[in] size size of the shared memory segment being instantiated/opened
    /// \param[in] mode mode in which shared memory segment is being opened/created
    /// \param[in] baseaddr address in the caller's memory address space this shared memory
    ///                      will be mapped to
    /// \param[in] ext_buff true if the shared memory is created on an external buffer
    ///                     false otherwise
    static shmmgr *factory(const char *name, const std::size_t size,
                           shm_mode_e mode, void *baseaddr, bool ext_buff);
    static void destroy(shmmgr *mmgr);
    static void remove(const char *name);
    static bool exists(const char *name, void *baseaddr = NULL);

    // allocate memory of give size (in bytes)from underlying system shared
    // memory.  if alignment is 0, caller doesn't care if memory allocated is
    // aligned or not. if the alignment is non-zero, it should be a power of 2
    // and >= 4
    void *alloc(const std::size_t size, const std::size_t alignment = 4,
                bool reset = false);

    // free the given memory back to system shared memory
    void free(void *mem);

    // returns the size of this shared memory segment
    std::size_t size(void) const;

    // returns the size of the free memory in this segment
    std::size_t free_size(void) const;

    // return the underlying memory manager
    // NOTE: use it only in exception cases
    void *mmgr(void) const;

    // find a named segment from shared memory. if 'create' is true it allocates
    // a new one for the specified size(should be > 0). otherwise it find and
    // return the pointer to the already existing segment.
    // if size is given in the latter case, it compares the allocated and the
    // requested and returns memory only if requested size <= allocated size
    void *segment_find(const char *name, bool create, std::size_t size = 0,
                       std::size_t alignment = 0);
    // get size of the named segment
    std::size_t get_segment_size(const char *name) const;

    // check if segment exists
    bool segment_exists(const char *name) const;

    // free a segment if exists
    bool segment_free(const char *name);

    // segment walk (iterate through all segments)
    void segment_walk(void *ctxt, shmmgr_seg_walk_cb_t cb);

    // get name
    const char *name(void) const { return name_; }
private:
    char    name_[SHMSEG_NAME_MAX_LEN+1];
    void    *mmgr_;
    bool    fixed_;
    bool    mapped_file_;
    bool    ext_buff_;

private:
    shmmgr();
    ~shmmgr();
    bool init(const char *name, const std::size_t size,
              shm_mode_e mode, void *baseaddr);
    bool ext_buff_init_(const std::size_t size, shm_mode_e mode,
                        void *baseaddr);
    template< typename T>
    void iterate(T *mmgr, void *ctx, shmmgr_seg_walk_cb_t cb);
};

}    // namespace lib
}    // namespace sdk

using sdk::lib::shmmgr;

#endif    // __SDK_SHMMGR_HPP__

