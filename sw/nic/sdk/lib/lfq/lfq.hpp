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
// lock free multi-producer, multi-consumer queue
//------------------------------------------------------------------------------

#ifndef __LFQ_HPP__
#define __LFQ_HPP__

#include "include/sdk/base.hpp"

namespace sdk {
namespace lib {

class lfq {
public:
    static lfq *factory(uint32_t size=0);
    static void destroy(lfq *q);
    bool enqueue(void *item);
    void *dequeue(void);

private:
    // forward declaration
    class lfq_impl;
    lfq_impl *lfq_impl_;

private:
    lfq();
    ~lfq() {}
};

}    // namespace utils
}    // namespace sdk

#endif    // __LFQ_HPP__

