// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <ospray/ospcommon/range.h>
#include <ospray/ospcommon/utility/ArrayView.h>
#include "common/ManagedObject.h"
#include "common/objectFactory.h"

using namespace ospcommon;

namespace volley {
  namespace ispc_driver {

    struct SamplesMask : public ManagedObject
    {
      SamplesMask()                   = default;
      virtual ~SamplesMask() override = default;

      virtual void commit() override
      {
        ManagedObject::commit();
      }

      void setRanges(const utility::ArrayView<const range1f> &ranges);

     protected:
      std::vector<range1f> ranges;
    };

  }  // namespace ispc_driver
}  // namespace volley
