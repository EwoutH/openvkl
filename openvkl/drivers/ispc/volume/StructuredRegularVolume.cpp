// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "StructuredRegularVolume.h"
#include "GridAccelerator_ispc.h"
#include "ospcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    StructuredRegularVolume<W>::~StructuredRegularVolume()
    {
      if (this->ispcEquivalent) {
        ispc::SharedStructuredVolume_Destructor(this->ispcEquivalent);
      }
    }

    template <int W>
    void StructuredRegularVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      voxelData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "voxelData", nullptr);

      if (!voxelData) {
        throw std::runtime_error("no voxelData set on volume");
      }

      if (voxelData->size() != longProduct(this->dimensions)) {
        throw std::runtime_error(
            "incorrect voxelData size for provided volume dimensions");
      }

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = ispc::SharedStructuredVolume_Constructor();

        if (!this->ispcEquivalent) {
          throw std::runtime_error(
              "could not create ISPC-side object for StructuredRegularVolume");
        }
      }

      bool success = ispc::SharedStructuredVolume_set(
          this->ispcEquivalent,
          voxelData->data,
          voxelData->dataType,
          (const ispc::vec3i &)this->dimensions,
          ispc::structured_regular,
          (const ispc::vec3f &)this->gridOrigin,
          (const ispc::vec3f &)this->gridSpacing);

      if (!success) {
        ispc::SharedStructuredVolume_Destructor(this->ispcEquivalent);
        this->ispcEquivalent = nullptr;

        throw std::runtime_error("failed to commit StructuredRegularVolume");
      }

      buildAccelerator();
    }

    template <int W>
    void StructuredRegularVolume<W>::buildAccelerator()
    {
      void *accelerator =
          ispc::SharedStructuredVolume_createAccelerator(this->ispcEquivalent);

      vec3i bricksPerDimension;
      bricksPerDimension.x =
          ispc::GridAccelerator_getBricksPerDimension_x(accelerator);
      bricksPerDimension.y =
          ispc::GridAccelerator_getBricksPerDimension_y(accelerator);
      bricksPerDimension.z =
          ispc::GridAccelerator_getBricksPerDimension_z(accelerator);

      const int numTasks =
          bricksPerDimension.x * bricksPerDimension.y * bricksPerDimension.z;
      tasking::parallel_for(numTasks, [&](int taskIndex) {
        ispc::GridAccelerator_build(accelerator, taskIndex);
      });
    }

    VKL_REGISTER_VOLUME(StructuredRegularVolume<4>, structured_regular_4)
    VKL_REGISTER_VOLUME(StructuredRegularVolume<8>, structured_regular_8)
    VKL_REGISTER_VOLUME(StructuredRegularVolume<16>, structured_regular_16)

  }  // namespace ispc_driver
}  // namespace openvkl
