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


#ifndef __PENIPC_AI_HPP__
#define __PENIPC_AI_HPP__

#include <memory>

#include "ipc.hpp"

#define AI_MSG_SIZE     4096

namespace sdk {
namespace ipc {

class ai_client {
 public:
     static std::shared_ptr<ai_client> create(uint32_t card_id);
     virtual ipc_msg_ptr request(uint32_t endpoint, uint32_t opcode,
             void *data, size_t length) = 0;

};
using ai_client_ptr = std::shared_ptr<ai_client>;

} // namespace ipc
} // namespace sdk

#endif
