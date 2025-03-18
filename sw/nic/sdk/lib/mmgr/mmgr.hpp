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


#ifndef __SDK_MMGR_HPP__
#define __SDK_MMGR_HPP__

#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/param.h>
#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"
#include "lib/logger/logger.h"

#define offsetofv1(type, member)       __builtin_offsetof(type, member)
#define containerof(ptr, type, member)    ((type *)(((char *)(ptr)) - offsetofv1(type, member)))

//------------------------------------------------------------------------------
// NOTE:
// this memory manager can work in the following two schemes :
//
// 1. pointer scheme - use pointers for meta data
//
// if the intended use of this library is across process restarts/upgrades, in
// pointer scheme one must ensure that the memory is mapped to same exact start
// address across restarts/upgrades.
//
// 2. offset scheme - use offsets for meta data
//
// if the intended use of this library is across process restarts/upgrades, but
// memory cannot or need not be mapped to same exact start address across
// restarts/uprades, then use the offset scheme. In this scheme, all meta data
// is stored as offsets from the beginning of the given start address and hence
// the value of the start address itself doesn't matter
//
// set MMGR_OFFSET_SCHEME to 1 when shared memory is not mapped to same virtual
// address all time
//------------------------------------------------------------------------------
#define MMGR_SCHEME_OFFSET
#ifdef MMGR_SCHEME_OFFSET
#define tnode_ptr_t                uint64_t
#define mblock_ptr_t               uint64_t
#define TO_TNODE_PTR(x)            ((tnode_t *)offset2ptr(x))
#define TO_TNODE_OFFSET(x)         ptr2offset(x)
#define TO_MBLOCK_PTR(x)           ((mblock_t *)offset2ptr(x))
#define TO_MBLOCK_OFFSET(x)        ptr2offset(x)
#else
#define tnode_ptr_t                tnode_t *
#define mblock_ptr_t               mblock_t *
#define TO_TNODE_PTR(x)            x
#define TO_TNODE_OFFSET(x)         x
#define TO_MBLOCK_PTR(x)           ((mblock_t *)(x))
#define TO_MBLOCK_OFFSET(x)        x
#endif

// tree node (both length tree and pointer tree use same node type)
typedef struct tnode_s    tnode_t;
typedef struct mblock_s   mblock_t;
struct tnode_s {
    uint64_t       key;      // key is either the length of the memory block
                             // (in length tree) or address offset (from the
                             // given start address we are managing) in the
                             // pointer tree
    tnode_ptr_t    left;
    tnode_ptr_t    right;
} __PACK__;

// per block context
struct mblock_s {
    tnode_t         ltree_node;    // node in the length tree (aka. ltree)
    tnode_t         ptree_node;    // node in the pointer tree (aka. ptree)
    mblock_ptr_t    prev;          // previous block (used for ltree)
    mblock_ptr_t    next;          // next block (used for ltree)
} __PACK__;
#define MBLOCK_SZ                sizeof(mblock_t)

// header of the memory manager
typedef struct mm_hdr_s {
    tnode_ptr_t    ltree;         // root of the length tree
    tnode_ptr_t    ptree;         // root of the pointer tree
    uint64_t       free_size;     // free memory available in bytes
    uint64_t       alloc_size;    // allocated memory in bytes
} __PACK__ mm_hdr_t;
#define MM_HDR_SZ                sizeof(mm_hdr_t)

class mmgr {
public:
    mmgr(void *mem, uint64_t size);
    ~mmgr();

    void *alloc(uint32_t len);
    void free(void *ptr);
    uint64_t free_size(void) const { return mm_hdr_->free_size; }
    uint64_t count_free_size(void) const;
    uint64_t allocated_size(void) const { return mm_hdr_->alloc_size; }
    uint64_t get_max_size(void) const {
                 return size_ - (MM_HDR_SZ + MBLOCK_SZ);
    }

    void preorder(void) const;    // TODO: add callback later on !!
    int height_ltree(void) const;
    int height_ptree(void) const;
    void level_order_ltree(void) const;
    void level_order_ptree(void) const;
    void set_root(tnode_ptr_t root) { mm_hdr_->ltree = root; }
    tnode_ptr_t splay_test(tnode_ptr_t *root, uint64_t key);

private:
    // TODO: only one of mem_ and mm_hdr_ is needed here !!
    uint8_t               *mem_;       // start of the memory block provided
    uint64_t              size_;       // total size of the memory block provided
    mm_hdr_t              *mm_hdr_;    // root memory header that has map of free and used memory
    pthread_spinlock_t    slock_;      // lock for synchornization
    uint32_t              oom_;        // out-of-condition counter

private:
    tnode_ptr_t splay(tnode_ptr_t *root, uint64_t key);
    void insert_node(tnode_ptr_t *root, tnode_t *node);
    void insert_node(tnode_ptr_t *root, tnode_t *node1, tnode_t *node2);
    void delete_node(tnode_ptr_t *root, tnode_t *node, bool splayed);
    void insert_ltree_node(tnode_ptr_t *root, tnode_t *node);
    void delete_ltree_node(tnode_ptr_t *root, tnode_t *node, bool splayed);

    // debug stuff
    void preorder_ltree_walk(tnode_t *node) const;
    void preorder_ptree_walk(tnode_t *node) const;
    int height(tnode_t *node) const;
    void level_walk(tnode_t *node, uint32_t level, bool ptree) const;
    void level_order_walk(tnode_t *tnode, bool ptree) const;
    tnode_t *right_rotate(tnode_t *node);
    tnode_t *left_rotate(tnode_t *node);
    tnode_ptr_t splay_r(tnode_ptr_t *root, uint64_t key);
    uint64_t count_free_size_of_tree(tnode_ptr_t node) const;

    bool is_valid(void *ptr) const {
        return ((ptr >= mem_) && (ptr < (mem_ + size_)));
    }
    uint64_t ptr2offset(void *ptr) const {
        assert(is_valid(ptr) == true);
        return (((uint8_t *)ptr) - mem_);
    }
    void *offset2ptr(uint64_t offset) const {
        assert(offset < size_);
        return ((void *)(mem_ + offset));
    }
};

#endif    // __SDK_MMGR_HPP__

