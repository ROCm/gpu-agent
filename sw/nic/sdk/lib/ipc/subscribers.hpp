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


#ifndef __SDK_IPC_SUBSCRIBERS_H__
#define __SDK_IPC_SUBSCRIBERS_H__

#include <vector>

#include <stdint.h>

#include "ipc.hpp"

namespace sdk {
namespace ipc {

class subscribers {
public:
    void set(uint32_t code, uint32_t client);
    void clear(uint32_t code, uint32_t client);
    std::vector<uint32_t> get(uint32_t code);
    static subscribers *instance(void);
private:
    static subscribers *instance_;
    subscribers();
    ~subscribers();
    void shmopen(void);
    uint8_t *subs_;
};

} // namespace sdk
} // namespace ipc

#endif // __SDK_IPC_SUBSCRIBERS_H__
