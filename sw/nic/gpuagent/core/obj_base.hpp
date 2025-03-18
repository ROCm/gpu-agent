
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


//----------------------------------------------------------------------------
///
/// \file
/// base object definition for all objects in the agent
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_OBJ_BASE_HPP__
#define __AGA_CORE_OBJ_BASE_HPP__

namespace aga {

/// \brief base class for all objects
class obj_base {
public:
    /// \brief constructor
    obj_base(){};

    /// \brief destructor
    virtual ~obj_base() {};
};

}    // namespace aga

using aga::obj_base;

#endif    // __AGA_CORE_OBJ_BASE_HPP__
