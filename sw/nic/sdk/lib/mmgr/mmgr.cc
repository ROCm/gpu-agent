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


#include <string.h>
#include "lib/mmgr/mmgr.hpp"

// TODO: take this as argument
// 4 byte alignment
#define MMGR_MEM_ALIGNMENT            4

//------------------------------------------------------------------------------
// memory manager constructor
//------------------------------------------------------------------------------
mmgr::mmgr(void *mem, uint64_t size) {
    mblock_t    *block;

    assert(mem != NULL);
    // TODO: this is not entirely accurate (based on start address we may or may
    // not need alignment added here !!!)
    assert(size >= MM_HDR_SZ + MBLOCK_SZ + MMGR_MEM_ALIGNMENT);
    mem_ = (uint8_t *)mem;
    size_ = size;

    // use 1st few bytes for the root header
    mm_hdr_ = (mm_hdr_t *)mem_;
    memset(mm_hdr_, 0, MM_HDR_SZ);
    mm_hdr_->free_size = get_max_size();

    pthread_spin_init(&slock_, PTHREAD_PROCESS_PRIVATE);
    oom_ = 0;

    // initialize the 1st block after root header
    block = (mblock_t *)(mm_hdr_ + 1);
    block->ptree_node.key = ptr2offset(block);
    block->ltree_node.key = mm_hdr_->free_size;
    insert_ltree_node(&mm_hdr_->ltree, &block->ltree_node);
    insert_node(&mm_hdr_->ptree, &block->ptree_node);
}

//------------------------------------------------------------------------------
// memory manager destructor
// most of the state is embedded in the free memory itself,
// so nothing much to cleanup
//------------------------------------------------------------------------------
mmgr::~mmgr() {
    pthread_spin_destroy(&slock_);
}

//------------------------------------------------------------------------------
// delete given node from the tree and if its not already splayed, splay it with
// key of the node being deleted
//------------------------------------------------------------------------------
void
mmgr::delete_node(tnode_ptr_t *root, tnode_t *node, bool splayed) {
    tnode_ptr_t    rtnode;

    if (!root || !node) {
        return;
    }
    rtnode = *root;

    if (splayed) {
        rtnode = TO_TNODE_OFFSET(node);
    } else {
        rtnode = splay(&rtnode, node->key);
    }

    if (TO_TNODE_PTR(rtnode)->key == node->key) {
        if (node->left == 0) {
            rtnode = node->right;
        } else {
            rtnode = splay(&node->left, node->key);
            TO_TNODE_PTR(rtnode)->right = node->right;
        }
    }
    *root = rtnode;
}

//------------------------------------------------------------------------------
// delete the given node from the length tree
//------------------------------------------------------------------------------
void
mmgr::delete_ltree_node(tnode_ptr_t *root, tnode_t *node, bool splayed) {
    mblock_t       *mblock, *mblock_next;
    tnode_ptr_t    rtnode;

    if (!root || !node) {
        return;
    }

    mblock = containerof(node, mblock_t, ltree_node);
    if (mblock->prev) {
        // this block is non-first block in the list, remove from the list
        TO_MBLOCK_PTR(mblock->prev)->next = mblock->next;
        if (mblock->next) {
            TO_MBLOCK_PTR(mblock->next)->prev = mblock->prev;
        }
    } else {
        // this block is the first block in the list, we must
        // 1. delete this node from the tree
        // 2. add next node in the list to the tree
        if (mblock->next == 0) {
            // this is the first and last node, simply delete
            delete_node(root, node, false);
        } else {
            // TODO: where is the delete of this node happening in this case ??
            mblock_next = TO_MBLOCK_PTR(mblock->next);
            rtnode = *root;
            if (splayed) {
                rtnode = TO_TNODE_OFFSET(node);
            } else {
                rtnode = splay(&rtnode, node->key);
            }
            if (rtnode && (TO_TNODE_PTR(rtnode)->key == node->key)) {
                mblock_next->ltree_node.left = TO_TNODE_PTR(rtnode)->left;
                mblock_next->ltree_node.right = TO_TNODE_PTR(rtnode)->right;
                mblock_next->prev = 0;
                *root = (tnode_ptr_t)(mblock->next);   // TODO: use container_of or offsetof()
            }
        }
    }
}

//------------------------------------------------------------------------------
// helper function to insert given node into the tree
//------------------------------------------------------------------------------
void
mmgr::insert_node(tnode_ptr_t *root, tnode_t *node1, tnode_t *node2) {
    if (node2 == NULL) {
        node1->left = node1->right = 0;
    } else if (node1->key < node2->key) {
        node1->left = node2->left;
        node1->right = TO_TNODE_OFFSET(node2);
        node2->left = 0;
    } else {
        node1->right = node2->right;
        node1->left = TO_TNODE_OFFSET(node2);
        node2->right = 0;
    }

    *root = TO_TNODE_OFFSET(node1);
}

//------------------------------------------------------------------------------
// helper function to insert given node into the tree
//------------------------------------------------------------------------------
void
mmgr::insert_node(tnode_ptr_t *root, tnode_t *node) {
    insert_node(root, node, TO_TNODE_PTR(splay(root, node->key)));
}

//------------------------------------------------------------------------------
// insert given node into length tree
//------------------------------------------------------------------------------
void
mmgr::insert_ltree_node(tnode_ptr_t *root, tnode_t *node) {
    mblock_t    *rmblock, *mblock;

    rmblock = TO_MBLOCK_PTR(splay(root, node->key));    // TODO: use container_of
    mblock = containerof(node, mblock_t, ltree_node);
    if ((rmblock == 0) || (rmblock->ltree_node.key != node->key)) {
        // insert into the tree as first node in the list
        mblock->prev = mblock->next = 0;
        insert_node(root, node, (tnode_t *)rmblock);    // TODO: use container_of
    } else {
        // add as a linked list element to existing node
        mblock->next = rmblock->next;
        mblock->prev = TO_MBLOCK_OFFSET(rmblock);
        rmblock->next = TO_MBLOCK_OFFSET(mblock);
        if (mblock->next) {
            TO_MBLOCK_PTR(mblock->next)->prev = TO_MBLOCK_OFFSET(mblock);
        }
    }
}

//------------------------------------------------------------------------------
// allocate requested amount of memory, length requested is rounded upto the
// alignment desired
//------------------------------------------------------------------------------
void *
mmgr::alloc(uint32_t len)  {
    void        *ptr = NULL;
    bool        splayed = true;
    mblock_t    *mblock, *mblock_new;
    uint32_t    total_len, rem;
    tnode_t     *node;

    if (!len) {
        return NULL;
    }

    // account for the alignment
    len = (len + (MMGR_MEM_ALIGNMENT - 1)) & ~(MMGR_MEM_ALIGNMENT - 1);

    pthread_spin_lock(&slock_);
    // check if we have free space
    if (mm_hdr_->free_size < (len + MBLOCK_SZ)) {
        // out of memory
        oom_++;
        goto end;
    }

    // splay the length tree to get the best match
    node = TO_TNODE_PTR(splay(&mm_hdr_->ltree, len));    // TODO: why is this not len + MBLOCK_SZ ??
    if (node && (len > node->key)) {
        splayed = false;
        // what we are looking for is on the right branch
        node = (tnode_t *)node->right;
        if (node) {
            while (node->left) {
                node = (tnode_t *)(node->left);
            }
        }
    }

    if (node == NULL) {
        // no node available in length tree with size >= requested length,
        // out of memory !!!
        oom_++;
        goto end;
    }

    mblock = containerof(node, mblock_t, ltree_node);
    total_len = len + MBLOCK_SZ;
    rem = node->key - total_len;
    ptr = (void *)(mblock + 1);

    // remove the block found from both length tree and pointer tree
    delete_ltree_node(&mm_hdr_->ltree, node, splayed);
    delete_node(&mm_hdr_->ptree, &mblock->ptree_node, false);

    // split the block found
    if (rem >= MMGR_MEM_ALIGNMENT) {   // TODO: why is this MMGR_MEM_ALIGNMENT ?
                                       // instead of rem > MMGR_MEM_ALIGNMENT + MBLOCK_SZ ?
        // plant new block after allocated memory segment
        mblock_new = (mblock_t *)((uint8_t *)ptr + len);
        mblock->ltree_node.key = len;
        mblock_new->ltree_node.key = rem;
        mblock_new->ptree_node.key = ptr2offset(mblock_new);

        // insert this new block now
        insert_ltree_node(&mm_hdr_->ltree, &mblock_new->ltree_node);
        insert_node(&mm_hdr_->ptree, &mblock_new->ptree_node);
    } else {
        total_len = node->key;
    }

    mblock->ltree_node.left = 0;
    mblock->ltree_node.right = 0;
    mblock->ptree_node.left = 0;
    mblock->ptree_node.right = 0;

    // update the memory mgr global info
    mm_hdr_->free_size -= total_len;
    mm_hdr_->alloc_size += total_len;

end:

    pthread_spin_unlock(&slock_);
    return ptr;   // TODO: this implies mblock is hanging even after deleting
                  // from both trees ? is there a leak here ?
}

//------------------------------------------------------------------------------
// free memory
//------------------------------------------------------------------------------
void
mmgr::free(void *ptr) {
    mblock_t       *mblock, *mblock_next;
    tnode_t        *node = NULL;
    tnode_ptr_t    pred = 0;

    if ((ptr == NULL) || !is_valid(ptr)) {
        return;
    }

    mblock = (mblock_t *)(((uint8_t *)ptr) - MBLOCK_SZ);
    mblock_next =
        (mblock_t *)(((uint8_t *)ptr) + mblock->ltree_node.key);

    pthread_spin_lock(&slock_);
    // TODO: we can catch double free here !!

    mm_hdr_->free_size += mblock->ltree_node.key;
    mm_hdr_->alloc_size -= mblock->ltree_node.key;

    if (is_valid(mblock_next))  {
        node = TO_TNODE_PTR(splay(&mm_hdr_->ptree, mblock_next->ptree_node.key));
    }
    if (node) {
        // find a predecessor
        if (node->key < mblock_next->ptree_node.key) {
            pred = TO_TNODE_OFFSET(node);
        } else if ((pred = node->left) > 0) {
            while (TO_TNODE_PTR(pred)->right) {
                pred = TO_TNODE_PTR(pred)->right;
            }
        }

        // check if we can merge this block with successor
        if (node->key == mblock_next->ptree_node.key) {
            // delete the successor from both trees
            delete_ltree_node(&mm_hdr_->ltree, &mblock_next->ltree_node, false);
            delete_node(&mm_hdr_->ptree, &mblock_next->ptree_node, false);

            // adjust the size
            mblock->ltree_node.key += mblock_next->ltree_node.key + MBLOCK_SZ;
            mm_hdr_->free_size += MBLOCK_SZ;
            mm_hdr_->alloc_size -= MBLOCK_SZ;
        }

        // check if the block can be merged with predecessor
        if (pred) {
            mblock_t *b = (mblock_t *)offset2ptr(TO_TNODE_PTR(pred)->key);
            mblock_t *t = (mblock_t *)(((uint8_t *)(b + 1)) + b->ltree_node.key);
            if (t->ptree_node.key == mblock->ptree_node.key) {
                // delete the predecessor from length tree
                delete_ltree_node(&mm_hdr_->ltree, &b->ltree_node, false);
                // adjust the size
                b->ltree_node.key += mblock->ltree_node.key + MBLOCK_SZ;
                mm_hdr_->free_size += MBLOCK_SZ;
                mm_hdr_->alloc_size -= MBLOCK_SZ;
                // re-insert the updated node in the length tree
                insert_ltree_node(&mm_hdr_->ltree, &b->ltree_node);
                mblock = NULL;
            }
        }

        // insert the node, if needed
        if (mblock) {
            insert_ltree_node(&mm_hdr_->ltree, &mblock->ltree_node);
            insert_node(&mm_hdr_->ptree, &mblock->ptree_node);
        }
    }

    pthread_spin_unlock(&slock_);
}

//------------------------------------------------------------------------------
// non recursive version of the splay operation around 'key', if the key is
// present in the tree, it brings the key to the root or else it brings the last
// accessed item to the root (note that the new root's key can be = or < or >
// than the key value
//
// NOTE: tree is modified and new root is returned
//------------------------------------------------------------------------------
tnode_ptr_t
mmgr::splay(tnode_ptr_t *root, uint64_t key) {
    tnode_t    *tnode, *left, *right, *tmp;
    tnode_t    node = { 0 };

    if (root == NULL) {
        return 0;
    }
    tnode = TO_TNODE_PTR(*root);
    if (tnode == NULL) {
        return 0;
    }
    left = right = &node;

    while (1) {
        if (key < tnode->key) {
            if (tnode->left == 0) {
                break;
            }
            // need to rotate right
            if (key < TO_TNODE_PTR(tnode->left)->key) {
                tmp = TO_TNODE_PTR(tnode->left);
                tnode->left = tmp->right;
                tmp->right = TO_TNODE_OFFSET(tnode);
                tnode = tmp;
                if (tnode->left == 0) {
                    break;
                }
            }
            right->left = TO_TNODE_OFFSET(tnode);
            right = tnode;
            tnode = TO_TNODE_PTR(tnode->left);
        } else if (key > tnode->key) {
            if (tnode->right == 0) {
                break;
            }
            // need to rotate left
            if (key > TO_TNODE_PTR(tnode->right)->key) {
                tmp = TO_TNODE_PTR(tnode->right);
                tnode->right = tmp->left;
                tmp->left = TO_TNODE_OFFSET(tnode);
                tnode = tmp;
                
                if (tnode->right == 0) {
                    break;
                }
            }
            left->right = TO_TNODE_OFFSET(tnode);
            left = tnode;
            tnode = TO_TNODE_PTR(tnode->right);
        } else {
            break;
        }
    }

    // re-create the node
    left->right = 0;
    right->left = 0;
    left->right = tnode->left;
    right->left = tnode->right;
    tnode->left = node.right;
    tnode->right = node.left;
    *root = TO_TNODE_OFFSET(tnode);
    return *root;
}

//------------------------------------------------------------------------------
// right rotate the tree rooted at 'node' and return node's left as new root
//------------------------------------------------------------------------------
tnode_t *
mmgr::right_rotate(tnode_t *node) {
    tnode_t    *new_root;

    new_root = TO_TNODE_PTR(node->left);
    node->left = new_root->right;
    new_root->right = TO_TNODE_OFFSET(node);
    return new_root;
}
 
//------------------------------------------------------------------------------
// left rotate the tree rooted at 'node' and return node's right as new root
//------------------------------------------------------------------------------
tnode_t *
mmgr::left_rotate(tnode_t *node) {
    tnode_t    *new_root;

    new_root = TO_TNODE_PTR(node->right);
    node->right = new_root->left;
    new_root->left = TO_TNODE_OFFSET(node);
    return new_root;
}
 
//------------------------------------------------------------------------------
// recursive version of the splay operation around 'key'. if the key is present
// in the tree, it brings the key to the root or else it brings the last
// accessed item to the root (note that the new root's key can be = or < or >
// than the key value
// NOTE: tree is recursively modified and new root is returned
//------------------------------------------------------------------------------
tnode_ptr_t
mmgr::splay_r(tnode_ptr_t *root, uint64_t key)
{
    tnode_t    *node;

    if ((root == NULL) || ((node = TO_TNODE_PTR(*root)) == NULL)) {
        return 0;
    }

    if (node->key == key) {
        // found what we are looking for
        return (*root = TO_TNODE_OFFSET(node));
    }
    
    if (node->key > key) {
        // key is in the left subtree
        if (node->left == 0) {
            // key not present, return last accessed node
            return (*root = TO_TNODE_OFFSET(node));
        }
        if (TO_TNODE_PTR(node->left)->key > key) {
            // recursively splay the left subtree of node->left->left
            //TO_TNODE_PTR(node->left)->left =
                splay_r(&TO_TNODE_PTR(node->left)->left, key);
            // now that the subtree is "fixed", right rotate the tree
            // rooted at node and bring its left subtree's root in its place
            node = right_rotate(node);
        } else if (TO_TNODE_PTR(node->left)->key < key) {
            // recursively splay the right subtree of node->left->right
            //TO_TNODE_PTR(node->left)->right =
                splay_r(&TO_TNODE_PTR(node->left)->right, key);
            if (TO_TNODE_PTR(node->left)->right != 0) {
                node->left =
                    TO_TNODE_OFFSET(left_rotate(TO_TNODE_PTR(node->left)));
            }
        }
        // do second rotation for root node now
        if (node->left) {
            *root = TO_TNODE_OFFSET(right_rotate(node));
        } else {
            *root = TO_TNODE_OFFSET(node);
        }
        return *root;
        //return ((node->left == 0) ? node : TO_TNODE_OFFSET(right_rotate(node)));
    } else {    // key is in the right subtree
        if (node->right == 0) {
            return (*root = TO_TNODE_OFFSET(node));
        }
        if (TO_TNODE_PTR(node->right)->key > key) {
            //TO_TNODE_PTR(node->right)->left =
                splay_r(&TO_TNODE_PTR(node->right)->left, key);
            if (TO_TNODE_PTR(node->right)->left != 0) {
                node->right =
                    TO_TNODE_OFFSET(right_rotate(TO_TNODE_PTR(node->right)));
            }
        } else if (TO_TNODE_PTR(node->right)->key < key) {
            TO_TNODE_PTR(node->right)->right =
                splay_r(&TO_TNODE_PTR(node->right)->right, key);
            node = left_rotate(node);
        }
        if (node->right) {
            *root = TO_TNODE_OFFSET(left_rotate(node));
        } else {
            *root = TO_TNODE_OFFSET(node);
        }
        return *root;
        //return ((node->right == 0) ? node : TO_TNODE_OFFSET(left_rotate(node)));
    }
}

//------------------------------------------------------------------------------
// traverse the tree in the preorder fashion
// TODO: call a callback here instead of printing
//------------------------------------------------------------------------------
void
mmgr::preorder_ltree_walk(tnode_t *node) const {
    if (node != NULL) {
        SDK_TRACE_DEBUG("key = %lu ", node->key);
        preorder_ltree_walk(TO_TNODE_PTR(node->left));
        preorder_ltree_walk(TO_TNODE_PTR(node->right));
        SDK_TRACE_DEBUG("\n");
    }
}

void
mmgr::preorder_ptree_walk(tnode_t *node) const {
    if (node != NULL) {
        SDK_TRACE_DEBUG("ptr = %p ", offset2ptr(node->key));
        preorder_ptree_walk(TO_TNODE_PTR(node->left));
        preorder_ptree_walk(TO_TNODE_PTR(node->right));
        SDK_TRACE_DEBUG("\n");
    }
}

void
mmgr::preorder(void) const {
    SDK_TRACE_DEBUG("\n\t\tlength tree walk\n");
    preorder_ltree_walk(TO_TNODE_PTR(mm_hdr_->ltree));
    SDK_TRACE_DEBUG("\n\t\tpointer tree walk\n");
    preorder_ptree_walk(TO_TNODE_PTR(mm_hdr_->ptree));
    SDK_TRACE_DEBUG("\n");
}

//------------------------------------------------------------------------------
// return the height of the tree starting from given node
//------------------------------------------------------------------------------
int
mmgr::height(tnode_t *node) const {
    if (node == NULL) {
        return -1;
    }
    return 1 + MAX(height(TO_TNODE_PTR(node->left)),
                   height(TO_TNODE_PTR(node->right)));
}

//------------------------------------------------------------------------------
// return the height of the length tree
//------------------------------------------------------------------------------
int
mmgr::height_ltree(void) const {
    return height(TO_TNODE_PTR(mm_hdr_->ltree));
}

//------------------------------------------------------------------------------
// return the height of the pointer tree
//------------------------------------------------------------------------------
int
mmgr::height_ptree(void) const {
    return height(TO_TNODE_PTR(mm_hdr_->ptree));
}
 
//------------------------------------------------------------------------------
// dump nodes at given level
//------------------------------------------------------------------------------
void
mmgr::level_walk(tnode_t *node, uint32_t level, bool ptree) const
{
    if (node == NULL)  {
        return;
    }
    if (level == 0) {
        if (ptree) {
            SDK_TRACE_DEBUG("%p ", offset2ptr(node->key));
        } else {
            SDK_TRACE_DEBUG("%lu ", node->key);
        }
    } else if (level > 0) {
        level_walk(TO_TNODE_PTR(node->left), level - 1, ptree);
        level_walk(TO_TNODE_PTR(node->right), level - 1, ptree);
    }
}

//------------------------------------------------------------------------------
// dump the tree starting from the given root node in level order
//------------------------------------------------------------------------------
void
mmgr::level_order_walk(tnode_t *root, bool ptree) const
{
    int    h = height(root);
    int    level;

    //SDK_TRACE_DEBUG("height of the tree %d\n", h);
    for (level = 0; level <= h; level++) {
        SDK_TRACE_DEBUG("level %u ===> ", level);
        level_walk(root, level, ptree);
        SDK_TRACE_DEBUG("\n");
    }
}

//------------------------------------------------------------------------------
// dump the length tree in level order
//------------------------------------------------------------------------------
void
mmgr::level_order_ltree(void) const
{
    SDK_TRACE_DEBUG("=== level order of length tree ===\n");
    level_order_walk(TO_TNODE_PTR(mm_hdr_->ltree), false);
    SDK_TRACE_DEBUG("==================================\n");
}

//------------------------------------------------------------------------------
// dump the pointer tree in level order
//------------------------------------------------------------------------------
void
mmgr::level_order_ptree(void) const
{
    SDK_TRACE_DEBUG("=== level order of pointer tree ===\n");
    level_order_walk(TO_TNODE_PTR(mm_hdr_->ptree), true);
    SDK_TRACE_DEBUG("==================================\n");
}

//------------------------------------------------------------------------------
// traverse the length tree in recursive fashion and count the total amount of
// memory available; note that at each level of this length tree there can be
// multiple blocks of same size that are non-contiguous and are not merged
// TODO: write non-recursive version
//------------------------------------------------------------------------------
uint64_t
mmgr::count_free_size_of_tree(tnode_ptr_t node) const
{
    uint64_t    sz = 0;
    mblock_t    *mblock, *mblock_next;

    // count the size at current level
    mblock = containerof(node, mblock_t, ltree_node);
    sz += mblock->ltree_node.key; // + MBLOCK_SZ;
    mblock_next = TO_MBLOCK_PTR(mblock->next);
    while (mblock_next) {
        sz += mblock_next->ltree_node.key; // + MBLOCK_SZ;
        mblock_next = TO_MBLOCK_PTR(mblock->next);
    }
    if (TO_TNODE_PTR(node)->left) {
        sz += count_free_size_of_tree(TO_TNODE_PTR(node)->left);
    }
    if (TO_TNODE_PTR(node)->right) {
        sz += count_free_size_of_tree(TO_TNODE_PTR(node)->right);
    }

    return sz;
}

//------------------------------------------------------------------------------
// walk the length tree and count the amount of free memory available
// NOTE: this is debug API only, it validates that the size counted by walking
//       both length tree and pointree are same as the run time counter and
//       then returns that value
//------------------------------------------------------------------------------
uint64_t
mmgr::count_free_size(void) const
{
    uint64_t    counted_sz;

    counted_sz = count_free_size_of_tree(mm_hdr_->ltree);
    assert(counted_sz == free_size());
    counted_sz = count_free_size_of_tree(mm_hdr_->ltree);
    assert(counted_sz == free_size());

    return counted_sz;
}

//------------------------------------------------------------------------------
// splay the tree with given key and dump the tree in level order before and
// after splaying it
//------------------------------------------------------------------------------
tnode_ptr_t
mmgr::splay_test(tnode_ptr_t *root, uint64_t key)
{
    tnode_ptr_t    node;

    level_order_ltree();
    node = splay_r(&mm_hdr_->ltree, key);
    level_order_ltree();

    return node;
}
