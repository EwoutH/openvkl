// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "VolleyRenderer.h"
#include "VolleyRenderer_ispc.h"
#include "transferFunction/TransferFunction.h"

namespace ospray {

  VolleyRenderer::VolleyRenderer()
  {
    std::cout << "creating OSPRay ISPC device VolleyRenderer" << std::endl;

    setParam<std::string>("externalNameFromAPI", "volley");

    ispcEquivalent = ispc::VolleyRenderer_create(this);
  }

  std::string VolleyRenderer::toString() const
  {
    return "ospray::render::VolleyRenderer";
  }

  void VolleyRenderer::commit()
  {
    Renderer::commit();

    VLYVolume vlyVolume = (VLYVolume)getParamVoidPtr("vlyVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the Volley renderer!");

    ispc::VolleyRenderer_set(
        getIE(), (ispc::VolleyVolume *)vlyVolume, transferFunction->getIE());
  }

  OSP_REGISTER_RENDERER(VolleyRenderer, volley);

}  // namespace ospray